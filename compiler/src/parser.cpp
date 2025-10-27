#include "parser.h"
#include <stdexcept>

Parser::Parser(const std::vector<Token>& toks) : tokens(toks) {}

std::unique_ptr<ProgramNode> Parser::parse() {
    auto program = std::make_unique<ProgramNode>();
    
    // Parse all shader declarations until EOF
    while (current().type != TokenType::END_OF_FILE) {
        if (current().type == TokenType::SHADER) {
            program->declarations.push_back(parseShaderDecl());
        } else {
            throw std::runtime_error("Expected 'shader' declaration at line " + 
                                   std::to_string(current().line));
        }
    }
    
    return program;
}

Token& Parser::current() {
    if (position >= tokens.size()) {
        return tokens.back(); // Return EOF
    }
    return tokens[position];
}

Token& Parser::peek(int offset) {
    size_t pos = position + offset;
    if (pos >= tokens.size()) {
        return tokens.back();
    }
    return tokens[pos];
}

void Parser::advance() {
    if (position < tokens.size()) {
        position++;
    }
}

bool Parser::match(TokenType type) {
    if (current().type == type) {
        advance();
        return true;
    }
    return false;
}

void Parser::expect(TokenType type, const std::string& message) {
    if (!match(type)) {
        throw std::runtime_error("Parse error at line " + std::to_string(current().line) + 
                               ": " + message + " (got '" + current().value + "')");
    }
}

bool Parser::check(TokenType type) {
    return current().type == type;
}

bool Parser::isTypeToken(TokenType type) {
    return type == TokenType::VEC2 || type == TokenType::VEC3 || 
           type == TokenType::VEC4 || type == TokenType::MAT4 ||
           type == TokenType::FLOAT || type == TokenType::INT;
}

std::string Parser::parseType() {
    if (!isTypeToken(current().type)) {
        throw std::runtime_error("Expected type specifier at line " + 
                               std::to_string(current().line));
    }
    std::string type = current().value;
    advance();
    return type;
}

std::unique_ptr<ShaderDeclNode> Parser::parseShaderDecl() {
    auto node = std::make_unique<ShaderDeclNode>();
    
    // Expect 'shader' keyword
    expect(TokenType::SHADER, "Expected 'shader' keyword");
    
    // Read shader type (vertex or fragment)
    if (current().type == TokenType::VERTEX) {
        node->shaderType = "vertex";
        advance();
    } else if (current().type == TokenType::FRAGMENT) {
        node->shaderType = "fragment";
        advance();
    } else {
        throw std::runtime_error("Expected 'vertex' or 'fragment' at line " + 
                               std::to_string(current().line));
    }
    
    // Expect opening brace
    expect(TokenType::LBRACE, "Expected '{' after shader type");
    
    // Parse shader body
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        if (current().type == TokenType::INPUT) {
            advance(); // consume 'input'
            node->inputs.push_back(parseVariableDecl(true));
        } else if (current().type == TokenType::OUTPUT) {
            advance(); // consume 'output'
            node->outputs.push_back(parseVariableDecl(false));
        } else if (current().type == TokenType::MAIN) {
            advance(); // consume 'main'
            expect(TokenType::LBRACE, "Expected '{' after 'main'");
            
            // Parse statements in main block
            while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
                node->statements.push_back(parseStatement());
            }
            
            expect(TokenType::RBRACE, "Expected '}' after main block");
        } else {
            throw std::runtime_error("Unexpected token in shader body at line " + 
                                   std::to_string(current().line) + ": " + current().value);
        }
    }
    
    // Expect closing brace
    expect(TokenType::RBRACE, "Expected '}' at end of shader declaration");
    
    return node;
}

std::unique_ptr<VariableDeclNode> Parser::parseVariableDecl(bool isInput) {
    auto node = std::make_unique<VariableDeclNode>();
    
    // Parse type
    node->varType = parseType();
    
    // Parse identifier name
    if (current().type != TokenType::IDENTIFIER) {
        throw std::runtime_error("Expected identifier after type at line " + 
                               std::to_string(current().line));
    }
    node->name = current().value;
    advance();
    
    // Expect semicolon
    expect(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    
    return node;
}

std::unique_ptr<ASTNode> Parser::parseStatement() {
    // Parse assignment statement
    // Format: identifier = expression;
    // or: identifier.member = expression;
    
    auto target = parsePrimary(); // Parse left-hand side
    
    // Expect assignment operator
    expect(TokenType::ASSIGN, "Expected '=' in assignment");
    
    // Parse right-hand side expression
    auto value = parseExpression();
    
    // Expect semicolon
    expect(TokenType::SEMICOLON, "Expected ';' after statement");
    
    // Create assignment node
    auto assignment = std::make_unique<AssignmentNode>();
    assignment->target = std::move(target);
    assignment->value = std::move(value);
    
    return assignment;
}

std::unique_ptr<ASTNode> Parser::parseExpression() {
    // Parse addition and subtraction (lowest precedence binary operators)
    // Expression -> Term (('+' | '-') Term)*
    
    auto left = parseTerm();
    
    while (current().type == TokenType::PLUS || current().type == TokenType::MINUS) {
        std::string op = current().value;
        advance();
        
        auto right = parseTerm();
        
        auto binOp = std::make_unique<BinaryOpNode>();
        binOp->op = op;
        binOp->left = std::move(left);
        binOp->right = std::move(right);
        
        left = std::move(binOp);
    }
    
    return left;
}

std::unique_ptr<ASTNode> Parser::parseTerm() {
    // Parse multiplication and division (higher precedence than +/-)
    // Term -> Factor (('*' | '/') Factor)*
    
    auto left = parseFactor();
    
    while (current().type == TokenType::MULTIPLY || current().type == TokenType::DIVIDE) {
        std::string op = current().value;
        advance();
        
        auto right = parseFactor();
        
        auto binOp = std::make_unique<BinaryOpNode>();
        binOp->op = op;
        binOp->left = std::move(left);
        binOp->right = std::move(right);
        
        left = std::move(binOp);
    }
    
    return left;
}

std::unique_ptr<ASTNode> Parser::parseFactor() {
    // Factor is the same as Primary in this grammar
    // (no unary operators to handle here)
    return parsePrimary();
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {
    // Parse highest precedence items:
    // - Numbers
    // - Identifiers (with optional member access or function call)
    // - Type names used as constructors (vec2, vec3, vec4, etc.)
    // - Parenthesized expressions
    
    // Number literal
    if (current().type == TokenType::NUMBER) {
        auto literal = std::make_unique<LiteralNode>();
        literal->value = current().value;
        advance();
        return literal;
    }
    
    // Type constructor (vec2, vec3, vec4, mat4, etc. used as functions)
    if (isTypeToken(current().type)) {
        std::string typeName = current().value;
        advance();
        
        // Type constructors must be followed by parentheses
        if (current().type == TokenType::LPAREN) {
            return parseFunctionCall(typeName);
        } else {
            throw std::runtime_error("Expected '(' after type constructor '" + typeName + 
                                   "' at line " + std::to_string(current().line));
        }
    }
    
    // Identifier (could be variable, member access, or function call)
    if (current().type == TokenType::IDENTIFIER) {
        std::string name = current().value;
        advance();
        
        // Check for member access (e.g., position.xyz)
        if (current().type == TokenType::DOT) {
            advance(); // consume '.'
            
            if (current().type != TokenType::IDENTIFIER) {
                throw std::runtime_error("Expected member name after '.' at line " + 
                                       std::to_string(current().line));
            }
            
            std::string member = current().value;
            advance();
            
            auto memberAccess = std::make_unique<MemberAccessNode>();
            auto object = std::make_unique<IdentifierNode>();
            object->name = name;
            memberAccess->object = std::move(object);
            memberAccess->member = member;
            
            return memberAccess;
        }
        // Check for function call (e.g., vec4(...))
        else if (current().type == TokenType::LPAREN) {
            return parseFunctionCall(name);
        }
        // Just an identifier
        else {
            auto identifier = std::make_unique<IdentifierNode>();
            identifier->name = name;
            return identifier;
        }
    }
    
    // Parenthesized expression
    if (current().type == TokenType::LPAREN) {
        advance(); // consume '('
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    throw std::runtime_error("Unexpected token in expression at line " + 
                           std::to_string(current().line) + ": " + current().value);
}

std::unique_ptr<FunctionCallNode> Parser::parseFunctionCall(const std::string& funcName) {
    auto funcCall = std::make_unique<FunctionCallNode>();
    funcCall->functionName = funcName;
    
    // Expect opening parenthesis
    expect(TokenType::LPAREN, "Expected '(' after function name");
    
    // Parse arguments
    if (current().type != TokenType::RPAREN) {
        // Parse first argument
        funcCall->arguments.push_back(parseExpression());
        
        // Parse remaining arguments
        while (current().type == TokenType::COMMA) {
            advance(); // consume ','
            funcCall->arguments.push_back(parseExpression());
        }
    }
    
    // Expect closing parenthesis
    expect(TokenType::RPAREN, "Expected ')' after function arguments");
    
    return funcCall;
}