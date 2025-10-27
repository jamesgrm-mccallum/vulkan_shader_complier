#include "optimizer.h"
#include <cmath>
#include <sstream>
#include <algorithm>

Optimizer::Optimizer() {}

void Optimizer::optimize(ProgramNode* ast) {
    // Run optimization passes iteratively until no more changes
    // This ensures we reach a fixed point
    bool changed = true;
    int maxPasses = 10; // Safety limit to prevent infinite loops
    
    while (changed && stats.totalPasses < maxPasses) {
        changed = false;
        stats.totalPasses++;
        
        // Run optimization passes on each shader declaration
        for (auto& decl : ast->declarations) {
            if (decl->type == ASTNodeType::SHADER_DECL) {
                auto* shader = static_cast<ShaderDeclNode*>(decl.get());
                
                // Run passes on all statements
                for (auto& stmt : shader->statements) {
                    // Constant folding pass
                    bool foldChanged = constantFoldingPass(stmt.get());
                    changed |= foldChanged;
                    
                    // Algebraic simplification pass
                    bool algChanged = algebraicSimplificationPass(stmt.get());
                    changed |= algChanged;
                }
                
                // Dead code elimination pass (runs on whole shader)
                bool deadChanged = deadCodeEliminationPass(ast);
                changed |= deadChanged;
            }
        }
    }
}

bool Optimizer::constantFoldingPass(ASTNode* node) {
    if (!node) return false;
    
    bool changed = false;
    
    switch (node->type) {
        case ASTNodeType::ASSIGNMENT: {
            auto* assign = static_cast<AssignmentNode*>(node);
            
            // Try to fold constants in the value expression
            changed |= constantFoldingPass(assign->value.get());
            
            // After recursion, check if the value itself can be folded
            if (assign->value->type == ASTNodeType::BINARY_OP) {
                auto* binOp = static_cast<BinaryOpNode*>(assign->value.get());
                
                // Check if both operands are literals
                if (isLiteral(binOp->left.get()) && isLiteral(binOp->right.get())) {
                    float leftVal = getLiteralValue(binOp->left.get());
                    float rightVal = getLiteralValue(binOp->right.get());
                    
                    auto folded = foldBinaryOp(binOp->op, leftVal, rightVal);
                    if (folded) {
                        // Replace the entire value expression with the folded literal
                        assign->value = std::move(folded);
                        stats.constantsFolded++;
                        changed = true;
                    }
                }
            }
            break;
        }
        
        case ASTNodeType::BINARY_OP: {
            auto* binOp = static_cast<BinaryOpNode*>(node);
            
            // Recursively optimize children first
            changed |= constantFoldingPass(binOp->left.get());
            changed |= constantFoldingPass(binOp->right.get());
            
            // After recursion, check if we can fold this operation
            if (isLiteral(binOp->left.get()) && isLiteral(binOp->right.get())) {
                float leftVal = getLiteralValue(binOp->left.get());
                float rightVal = getLiteralValue(binOp->right.get());
                
                auto folded = foldBinaryOp(binOp->op, leftVal, rightVal);
                if (folded) {
                    // We can't replace ourselves, but we can transform into a literal
                    auto* foldedLit = static_cast<LiteralNode*>(folded.get());
                    
                    // Transform this BinaryOpNode into a LiteralNode
                    binOp->type = ASTNodeType::LITERAL;
                    binOp->left.reset();
                    binOp->right.reset();
                    binOp->op.clear();
                    
                    // Reinterpret as LiteralNode and set value
                    auto* litNode = reinterpret_cast<LiteralNode*>(binOp);
                    litNode->value = foldedLit->value;
                    
                    stats.constantsFolded++;
                    changed = true;
                }
            }
            break;
        }
        
        case ASTNodeType::FUNCTION_CALL: {
            auto* funcCall = static_cast<FunctionCallNode*>(node);
            for (auto& arg : funcCall->arguments) {
                changed |= constantFoldingPass(arg.get());
                
                // After recursion, check if argument can be folded
                if (arg->type == ASTNodeType::BINARY_OP) {
                    auto* binOp = static_cast<BinaryOpNode*>(arg.get());
                    if (isLiteral(binOp->left.get()) && isLiteral(binOp->right.get())) {
                        float leftVal = getLiteralValue(binOp->left.get());
                        float rightVal = getLiteralValue(binOp->right.get());
                        
                        auto folded = foldBinaryOp(binOp->op, leftVal, rightVal);
                        if (folded) {
                            arg = std::move(folded);
                            stats.constantsFolded++;
                            changed = true;
                        }
                    }
                }
            }
            break;
        }
        
        case ASTNodeType::MEMBER_ACCESS: {
            auto* member = static_cast<MemberAccessNode*>(node);
            changed |= constantFoldingPass(member->object.get());
            break;
        }
        
        default:
            break;
    }
    
    return changed;
}

bool Optimizer::deadCodeEliminationPass(ProgramNode* ast) {
    bool changed = false;
    
    for (auto& decl : ast->declarations) {
        if (decl->type == ASTNodeType::SHADER_DECL) {
            auto* shader = static_cast<ShaderDeclNode*>(decl.get());
            
            // Collect all declared variables (from inputs/outputs)
            std::set<std::string> declaredVars;
            collectDeclaredVariables(shader, declaredVars);
            
            // Collect all used variables
            std::set<std::string> usedVars;
            collectUsedVariablesInStatements(shader->statements, usedVars);
            
            // Also mark output variables as used (they're implicitly used)
            for (auto& output : shader->outputs) {
                if (output->type == ASTNodeType::VARIABLE_DECL) {
                    auto* varDecl = static_cast<VariableDeclNode*>(output.get());
                    usedVars.insert(varDecl->name);
                }
            }
            
            // Remove statements that assign to unused variables
            auto it = shader->statements.begin();
            while (it != shader->statements.end()) {
                bool shouldRemove = false;
                
                if ((*it)->type == ASTNodeType::ASSIGNMENT) {
                    auto* assign = static_cast<AssignmentNode*>(it->get());
                    
                    // Get the target variable name
                    std::string targetName;
                    if (assign->target->type == ASTNodeType::IDENTIFIER) {
                        auto* id = static_cast<IdentifierNode*>(assign->target.get());
                        targetName = id->name;
                    } else if (assign->target->type == ASTNodeType::MEMBER_ACCESS) {
                        auto* member = static_cast<MemberAccessNode*>(assign->target.get());
                        if (member->object->type == ASTNodeType::IDENTIFIER) {
                            auto* id = static_cast<IdentifierNode*>(member->object.get());
                            targetName = id->name;
                        }
                    }
                    
                    // Check if target is used and not an output variable
                    if (!targetName.empty() && 
                        usedVars.find(targetName) == usedVars.end() &&
                        !isOutputVariable(targetName, shader)) {
                        shouldRemove = true;
                        stats.deadCodeRemoved++;
                        changed = true;
                    }
                }
                
                if (shouldRemove) {
                    it = shader->statements.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    
    return changed;
}

bool Optimizer::algebraicSimplificationPass(ASTNode* node) {
    if (!node) return false;
    
    bool changed = false;
    
    switch (node->type) {
        case ASTNodeType::ASSIGNMENT: {
            auto* assign = static_cast<AssignmentNode*>(node);
            
            // Try to simplify the value expression
            if (assign->value->type == ASTNodeType::BINARY_OP) {
                auto* binOp = static_cast<BinaryOpNode*>(assign->value.get());
                
                // First, recursively simplify children
                changed |= algebraicSimplificationPass(binOp->left.get());
                changed |= algebraicSimplificationPass(binOp->right.get());
                
                // Try to simplify this binary operation
                auto simplified = simplifyBinaryOp(binOp, changed);
                if (simplified) {
                    assign->value = std::move(simplified);
                }
            } else {
                // Recursively simplify other expression types
                changed |= algebraicSimplificationPass(assign->value.get());
            }
            break;
        }
        
        case ASTNodeType::BINARY_OP: {
            auto* binOp = static_cast<BinaryOpNode*>(node);
            
            // Recursively simplify children
            changed |= algebraicSimplificationPass(binOp->left.get());
            changed |= algebraicSimplificationPass(binOp->right.get());
            break;
        }
        
        case ASTNodeType::FUNCTION_CALL: {
            auto* funcCall = static_cast<FunctionCallNode*>(node);
            for (auto& arg : funcCall->arguments) {
                // Try to simplify each argument
                if (arg->type == ASTNodeType::BINARY_OP) {
                    auto* binOp = static_cast<BinaryOpNode*>(arg.get());
                    changed |= algebraicSimplificationPass(binOp->left.get());
                    changed |= algebraicSimplificationPass(binOp->right.get());
                    
                    auto simplified = simplifyBinaryOp(binOp, changed);
                    if (simplified) {
                        arg = std::move(simplified);
                    }
                } else {
                    changed |= algebraicSimplificationPass(arg.get());
                }
            }
            break;
        }
        
        case ASTNodeType::MEMBER_ACCESS: {
            auto* member = static_cast<MemberAccessNode*>(node);
            changed |= algebraicSimplificationPass(member->object.get());
            break;
        }
        
        default:
            break;
    }
    
    return changed;
}

// Helper methods

bool Optimizer::isLiteral(ASTNode* node) {
    return node && node->type == ASTNodeType::LITERAL;
}

bool Optimizer::isLiteralValue(ASTNode* node, float value) {
    if (!isLiteral(node)) {
        return false;
    }
    
    auto* lit = static_cast<LiteralNode*>(node);
    float nodeValue = std::stof(lit->value);
    return std::abs(nodeValue - value) < 0.0001f;
}

float Optimizer::getLiteralValue(ASTNode* node) {
    if (!isLiteral(node)) {
        throw std::runtime_error("Node is not a literal!");
    }
    
    auto* lit = static_cast<LiteralNode*>(node);
    return std::stof(lit->value);
}

std::unique_ptr<ASTNode> Optimizer::foldBinaryOp(const std::string& op, float left, float right) {
    float result;
    
    if (op == "+") {
        result = left + right;
    } else if (op == "-") {
        result = left - right;
    } else if (op == "*") {
        result = left * right;
    } else if (op == "/") {
        if (std::abs(right) < 0.0001f) {
            return nullptr; // Don't fold division by zero
        }
        result = left / right;
    } else {
        return nullptr; // Unknown operator
    }
    
    auto literal = std::make_unique<LiteralNode>();
    std::ostringstream oss;
    oss << result;
    literal->value = oss.str();
    
    return literal;
}

void Optimizer::collectDeclaredVariables(ShaderDeclNode* shader, std::set<std::string>& declared) {
    for (auto& input : shader->inputs) {
        if (input->type == ASTNodeType::VARIABLE_DECL) {
            auto* varDecl = static_cast<VariableDeclNode*>(input.get());
            declared.insert(varDecl->name);
        }
    }
    
    for (auto& output : shader->outputs) {
        if (output->type == ASTNodeType::VARIABLE_DECL) {
            auto* varDecl = static_cast<VariableDeclNode*>(output.get());
            declared.insert(varDecl->name);
        }
    }
}

void Optimizer::collectUsedVariables(ASTNode* node, std::set<std::string>& used) {
    if (!node) return;
    
    switch (node->type) {
        case ASTNodeType::IDENTIFIER: {
            auto* id = static_cast<IdentifierNode*>(node);
            used.insert(id->name);
            break;
        }
        
        case ASTNodeType::BINARY_OP: {
            auto* binOp = static_cast<BinaryOpNode*>(node);
            collectUsedVariables(binOp->left.get(), used);
            collectUsedVariables(binOp->right.get(), used);
            break;
        }
        
        case ASTNodeType::MEMBER_ACCESS: {
            auto* member = static_cast<MemberAccessNode*>(node);
            collectUsedVariables(member->object.get(), used);
            break;
        }
        
        case ASTNodeType::FUNCTION_CALL: {
            auto* funcCall = static_cast<FunctionCallNode*>(node);
            for (auto& arg : funcCall->arguments) {
                collectUsedVariables(arg.get(), used);
            }
            break;
        }
        
        case ASTNodeType::ASSIGNMENT: {
            auto* assign = static_cast<AssignmentNode*>(node);
            // Only collect from the right-hand side (value)
            collectUsedVariables(assign->value.get(), used);
            break;
        }
        
        default:
            break;
    }
}

void Optimizer::collectUsedVariablesInStatements(const std::vector<std::unique_ptr<ASTNode>>& statements, 
                                                 std::set<std::string>& used) {
    for (auto& stmt : statements) {
        collectUsedVariables(stmt.get(), used);
    }
}

bool Optimizer::isOutputVariable(const std::string& varName, ShaderDeclNode* shader) {
    for (auto& output : shader->outputs) {
        if (output->type == ASTNodeType::VARIABLE_DECL) {
            auto* varDecl = static_cast<VariableDeclNode*>(output.get());
            if (varDecl->name == varName) {
                return true;
            }
        }
    }
    
    // Built-in output variables
    return varName == "gl_Position" || varName == "gl_FragColor" || 
           varName == "gl_FragDepth";
}

std::unique_ptr<ASTNode> Optimizer::simplifyBinaryOp(BinaryOpNode* node, bool& changed) {
    // Apply algebraic identities

    // Combine adjacent literals in associative chains like ((x * c1) * c2) -> (x * (c1*c2))
    // and ((x + c1) + c2) -> (x + (c1+c2)). This helps constant folding fire later.
    // Handle multiplication
    if (node->op == "*") {
        // Shape: ( (X * c1) * c2 )
        if (node->right && isLiteral(node->right.get())
            && node->left && node->left->type == ASTNodeType::BINARY_OP) {
            auto* leftBin = static_cast<BinaryOpNode*>(node->left.get());
            if (leftBin->op == "*" && leftBin->right && isLiteral(leftBin->right.get())) {
                float c1 = getLiteralValue(leftBin->right.get());
                float c2 = getLiteralValue(node->right.get());
                auto newNode = std::make_unique<BinaryOpNode>();
                newNode->op = "*";
                newNode->left = cloneNode(leftBin->left.get());
                auto combined = std::make_unique<LiteralNode>();
                {
                    std::ostringstream oss;
                    oss << (c1 * c2);
                    combined->value = oss.str();
                }
                newNode->right = std::move(combined);
                stats.algebraicSimplifications++; stats.constantsFolded++; changed = true; return newNode;
            }
        }
        // Symmetric shape: ( c1 * (X * c2) )
        if (node->left && isLiteral(node->left.get())
            && node->right && node->right->type == ASTNodeType::BINARY_OP) {
            auto* rightBin = static_cast<BinaryOpNode*>(node->right.get());
            if (rightBin->op == "*" && rightBin->right && isLiteral(rightBin->right.get())) {
                float c1 = getLiteralValue(node->left.get());
                float c2 = getLiteralValue(rightBin->right.get());
                auto newNode = std::make_unique<BinaryOpNode>();
                newNode->op = "*";
                newNode->left = cloneNode(rightBin->left.get());
                auto combined = std::make_unique<LiteralNode>();
                {
                    std::ostringstream oss;
                    oss << (c1 * c2);
                    combined->value = oss.str();
                }
                newNode->right = std::move(combined);
                stats.algebraicSimplifications++; stats.constantsFolded++; changed = true; return newNode;
            }
        }
    }
    // Handle addition
    if (node->op == "+") {
        // Shape: ( (X + c1) + c2 )
        if (node->right && isLiteral(node->right.get())
            && node->left && node->left->type == ASTNodeType::BINARY_OP) {
            auto* leftBin = static_cast<BinaryOpNode*>(node->left.get());
            if (leftBin->op == "+" && leftBin->right && isLiteral(leftBin->right.get())) {
                float c1 = getLiteralValue(leftBin->right.get());
                float c2 = getLiteralValue(node->right.get());
                auto newNode = std::make_unique<BinaryOpNode>();
                newNode->op = "+";
                newNode->left = cloneNode(leftBin->left.get());
                auto combined = std::make_unique<LiteralNode>();
                {
                    std::ostringstream oss;
                    oss << (c1 + c2);
                    combined->value = oss.str();
                }
                newNode->right = std::move(combined);
                stats.algebraicSimplifications++; stats.constantsFolded++; changed = true; return newNode;
            }
        }
        // Symmetric shape: ( c1 + (X + c2) )
        if (node->left && isLiteral(node->left.get())
            && node->right && node->right->type == ASTNodeType::BINARY_OP) {
            auto* rightBin = static_cast<BinaryOpNode*>(node->right.get());
            if (rightBin->op == "+" && rightBin->right && isLiteral(rightBin->right.get())) {
                float c1 = getLiteralValue(node->left.get());
                float c2 = getLiteralValue(rightBin->right.get());
                auto newNode = std::make_unique<BinaryOpNode>();
                newNode->op = "+";
                newNode->left = cloneNode(rightBin->left.get());
                auto combined = std::make_unique<LiteralNode>();
                {
                    std::ostringstream oss;
                    oss << (c1 + c2);
                    combined->value = oss.str();
                }
                newNode->right = std::move(combined);
                stats.algebraicSimplifications++; stats.constantsFolded++; changed = true; return newNode;
            }
        }
    }


    
    // Multiplication simplifications
    if (node->op == "*") {
        // x * 1 -> x
        if (isLiteralValue(node->right.get(), 1.0f)) {
            stats.algebraicSimplifications++;
            changed = true;
            return cloneNode(node->left.get());
        }
        // 1 * x -> x
        if (isLiteralValue(node->left.get(), 1.0f)) {
            stats.algebraicSimplifications++;
            changed = true;
            return cloneNode(node->right.get());
        }
        // x * 0 -> 0
        if (isLiteralValue(node->right.get(), 0.0f)) {
            stats.algebraicSimplifications++;
            changed = true;
            auto zero = std::make_unique<LiteralNode>();
            zero->value = "0.0";
            return zero;
        }
        // 0 * x -> 0
        if (isLiteralValue(node->left.get(), 0.0f)) {
            stats.algebraicSimplifications++;
            changed = true;
            auto zero = std::make_unique<LiteralNode>();
            zero->value = "0.0";
            return zero;
        }
    }
    
    // Addition simplifications
    if (node->op == "+") {
        // x + 0 -> x
        if (isLiteralValue(node->right.get(), 0.0f)) {
            stats.algebraicSimplifications++;
            changed = true;
            return cloneNode(node->left.get());
        }
        // 0 + x -> x
        if (isLiteralValue(node->left.get(), 0.0f)) {
            stats.algebraicSimplifications++;
            changed = true;
            return cloneNode(node->right.get());
        }
    }
    
    // Subtraction simplifications
    if (node->op == "-") {
        // x - 0 -> x
        if (isLiteralValue(node->right.get(), 0.0f)) {
            stats.algebraicSimplifications++;
            changed = true;
            return cloneNode(node->left.get());
        }
    }
    
    // Division simplifications
    if (node->op == "/") {
        // x / 1 -> x
        if (isLiteralValue(node->right.get(), 1.0f)) {
            stats.algebraicSimplifications++;
            changed = true;
            return cloneNode(node->left.get());
        }
    }
    
    return nullptr; // No simplification
}

std::unique_ptr<ASTNode> Optimizer::cloneNode(ASTNode* node) {
    if (!node) return nullptr;
    
    switch (node->type) {
        case ASTNodeType::LITERAL: {
            auto* lit = static_cast<LiteralNode*>(node);
            auto clone = std::make_unique<LiteralNode>();
            clone->value = lit->value;
            return clone;
        }
        
        case ASTNodeType::IDENTIFIER: {
            auto* id = static_cast<IdentifierNode*>(node);
            auto clone = std::make_unique<IdentifierNode>();
            clone->name = id->name;
            return clone;
        }
        
        case ASTNodeType::BINARY_OP: {
            auto* binOp = static_cast<BinaryOpNode*>(node);
            auto clone = std::make_unique<BinaryOpNode>();
            clone->op = binOp->op;
            clone->left = cloneNode(binOp->left.get());
            clone->right = cloneNode(binOp->right.get());
            return clone;
        }
        
        case ASTNodeType::MEMBER_ACCESS: {
            auto* member = static_cast<MemberAccessNode*>(node);
            auto clone = std::make_unique<MemberAccessNode>();
            clone->object = cloneNode(member->object.get());
            clone->member = member->member;
            return clone;
        }
        
        case ASTNodeType::FUNCTION_CALL: {
            auto* funcCall = static_cast<FunctionCallNode*>(node);
            auto clone = std::make_unique<FunctionCallNode>();
            clone->functionName = funcCall->functionName;
            for (auto& arg : funcCall->arguments) {
                clone->arguments.push_back(cloneNode(arg.get()));
            }
            return clone;
        }
        
        default:
            return nullptr;
    }
}