#include "shader_compiler.h"
#include "lexer.h"
#include "parser.h"
#include "optimizer.h"
#include "codegen.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <functional>

ShaderCompiler::ShaderCompiler() { optimizationEnabled = true; }

ShaderCompiler::~ShaderCompiler() {}

std::vector<uint32_t> ShaderCompiler::compile(const std::string& source, const std::string& shaderType) {
    // Reset stats for new compilation
    resetStats();
    
    // Validate shader type
    validateShaderType(shaderType);
    
    double totalStartTime = getCurrentTimeMs();
    
    try {
        // ====================================
        // PHASE 1: LEXICAL ANALYSIS (LEXING)
        // ====================================
        logVerbose("Starting lexical analysis...");
        double lexStartTime = getCurrentTimeMs();
        
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();
        
        double lexEndTime = getCurrentTimeMs();
        stats.lexingTimeMs = lexEndTime - lexStartTime;
        stats.tokenCount = tokens.size();
        
        logVerbose("Lexing complete: " + std::to_string(stats.tokenCount) + " tokens generated");
        
        // ====================================
        // PHASE 2: SYNTAX ANALYSIS (PARSING)
        // ====================================
        logVerbose("Starting syntax analysis...");
        double parseStartTime = getCurrentTimeMs();
        
        Parser parser(tokens);
        std::unique_ptr<ProgramNode> ast = parser.parse();
        
        if (!ast) {
            throw ShaderCompilationError(
                ShaderCompilationError::Stage::PARSING,
                "Parser returned null AST"
            );
        }
        
        double parseEndTime = getCurrentTimeMs();
        stats.parsingTimeMs = parseEndTime - parseStartTime;
        stats.astNodeCount = countASTNodes(ast.get());
        stats.originalStatementCount = countStatements(ast.get());
        
        logVerbose("Parsing complete: " + std::to_string(stats.astNodeCount) + 
                   " AST nodes, " + std::to_string(stats.originalStatementCount) + " statements");
        
        // ====================================
        // PHASE 3: OPTIMIZATION
        // ====================================
        if (optimizationEnabled) {
            logVerbose("Starting optimization passes...");
            double optStartTime = getCurrentTimeMs();
            
            Optimizer optimizer;
            optimizer.optimize(ast.get());
            
            double optEndTime = getCurrentTimeMs();
            stats.optimizationTimeMs = optEndTime - optStartTime;
            
            // Collect optimization statistics
            auto optStats = optimizer.getStats();
            stats.constantsFolded = optStats.constantsFolded;
            stats.deadCodeEliminated = optStats.deadCodeRemoved;
            stats.algebraicSimplifications = optStats.algebraicSimplifications;
            stats.optimizationPasses = optStats.totalPasses;
            stats.optimizedStatementCount = countStatements(ast.get());
            
            logVerbose("Optimization complete: " + std::to_string(stats.optimizationPasses) + 
                       " passes, " + std::to_string(stats.constantsFolded) + " constants folded, " +
                       std::to_string(stats.algebraicSimplifications) + " algebraic simplifications, " +
                       std::to_string(stats.deadCodeEliminated) + " dead code eliminated");
        } else {
            logVerbose("Optimization disabled, skipping...");
            stats.optimizedStatementCount = stats.originalStatementCount;
        }
        
        // ====================================
        // PHASE 4: CODE GENERATION
        // ====================================
        logVerbose("Starting code generation...");
        double codegenStartTime = getCurrentTimeMs();
        
        CodeGenerator codegen;
        std::vector<uint32_t> spirv = codegen.generate(ast.get(), shaderType);
        
        double codegenEndTime = getCurrentTimeMs();
        stats.codegenTimeMs = codegenEndTime - codegenStartTime;
        
        // Store generated GLSL for inspection
        generatedGLSL = codegen.getGeneratedGLSL();
        
        // Calculate SPIR-V statistics
        stats.spirvSizeBytes = spirv.size() * sizeof(uint32_t);
        stats.spirvInstructionCount = spirv.size();
        
        logVerbose("Code generation complete: " + std::to_string(stats.spirvSizeBytes) + 
                   " bytes SPIR-V, " + std::to_string(stats.spirvInstructionCount) + " words");
        
        // ====================================
        // COMPILATION COMPLETE
        // ====================================
        double totalEndTime = getCurrentTimeMs();
        stats.totalTimeMs = totalEndTime - totalStartTime;
        
        if (verbose) {
            std::cout << "\n=== Compilation Summary ===" << std::endl;
            std::cout << "Total time: " << stats.totalTimeMs << " ms" << std::endl;
            std::cout << "  Lexing: " << stats.lexingTimeMs << " ms" << std::endl;
            std::cout << "  Parsing: " << stats.parsingTimeMs << " ms" << std::endl;
            std::cout << "  Optimization: " << stats.optimizationTimeMs << " ms" << std::endl;
            std::cout << "  Code generation: " << stats.codegenTimeMs << " ms" << std::endl;
            std::cout << "Tokens: " << stats.tokenCount << std::endl;
            std::cout << "AST nodes: " << stats.astNodeCount << std::endl;
            std::cout << "Statements: " << stats.originalStatementCount << " -> " 
                      << stats.optimizedStatementCount << std::endl;
            std::cout << "SPIR-V size: " << stats.spirvSizeBytes << " bytes" << std::endl;
            std::cout << "==========================\n" << std::endl;
        }
        
        return spirv;
        
    } catch (const std::runtime_error& e) {
        // Determine which phase failed based on the exception message
        std::string errorMsg = e.what();
        
        if (errorMsg.find("Lexer") != std::string::npos || 
            errorMsg.find("token") != std::string::npos ||
            errorMsg.find("Unexpected character") != std::string::npos) {
            throw ShaderCompilationError(
                ShaderCompilationError::Stage::LEXING,
                errorMsg
            );
        } else if (errorMsg.find("Parse") != std::string::npos || 
                   errorMsg.find("Expected") != std::string::npos ||
                   errorMsg.find("syntax") != std::string::npos) {
            throw ShaderCompilationError(
                ShaderCompilationError::Stage::PARSING,
                errorMsg
            );
        } else if (errorMsg.find("Optimizer") != std::string::npos ||
                   errorMsg.find("optimization") != std::string::npos) {
            throw ShaderCompilationError(
                ShaderCompilationError::Stage::OPTIMIZATION,
                errorMsg
            );
        } else {
            throw ShaderCompilationError(
                ShaderCompilationError::Stage::CODE_GENERATION,
                errorMsg
            );
        }
    }
}

std::vector<uint32_t> ShaderCompiler::compileFromFile(const std::string& filename, 
                                                       const std::string& shaderType) {
    logVerbose("Loading shader from file: " + filename);
    
    // Open and read the file
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filename);
    }
    
    // Read entire file into string
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    file.close();
    
    if (source.empty()) {
        throw std::runtime_error("Shader file is empty: " + filename);
    }
    
    logVerbose("File loaded: " + std::to_string(source.length()) + " characters");
    
    // Compile the source
    return compile(source, shaderType);
}

void ShaderCompiler::resetStats() {
    stats = CompilationStats();
    generatedGLSL.clear();
}

bool ShaderCompiler::isValidShaderType(const std::string& shaderType) {
    return shaderType == "vertex" || shaderType == "fragment";
}

// Private helper methods

void ShaderCompiler::logVerbose(const std::string& message) {
    if (verbose) {
        std::cout << "[ShaderCompiler] " << message << std::endl;
    }
}

void ShaderCompiler::validateShaderType(const std::string& shaderType) {
    if (!isValidShaderType(shaderType)) {
        throw std::runtime_error(
            "Invalid shader type: '" + shaderType + "'. Must be 'vertex' or 'fragment'."
        );
    }
}

size_t ShaderCompiler::countASTNodes(const ProgramNode* ast) {
    if (!ast) return 0;
    
    size_t count = 1; // Count the program node itself
    
    // Count nodes recursively
    std::function<void(const ASTNode*)> countNode = [&](const ASTNode* node) {
        if (!node) return;
        
        count++;
        
        switch (node->type) {
            case ASTNodeType::SHADER_DECL: {
                auto* shader = static_cast<const ShaderDeclNode*>(node);
                for (auto& input : shader->inputs) countNode(input.get());
                for (auto& output : shader->outputs) countNode(output.get());
                for (auto& stmt : shader->statements) countNode(stmt.get());
                break;
            }
            case ASTNodeType::ASSIGNMENT: {
                auto* assign = static_cast<const AssignmentNode*>(node);
                countNode(assign->target.get());
                countNode(assign->value.get());
                break;
            }
            case ASTNodeType::BINARY_OP: {
                auto* binOp = static_cast<const BinaryOpNode*>(node);
                countNode(binOp->left.get());
                countNode(binOp->right.get());
                break;
            }
            case ASTNodeType::MEMBER_ACCESS: {
                auto* member = static_cast<const MemberAccessNode*>(node);
                countNode(member->object.get());
                break;
            }
            case ASTNodeType::FUNCTION_CALL: {
                auto* funcCall = static_cast<const FunctionCallNode*>(node);
                for (auto& arg : funcCall->arguments) countNode(arg.get());
                break;
            }
            default:
                break;
        }
    };
    
    for (auto& decl : ast->declarations) {
        countNode(decl.get());
    }
    
    return count;
}

size_t ShaderCompiler::countStatements(const ProgramNode* ast) {
    if (!ast) return 0;
    
    size_t count = 0;
    
    for (auto& decl : ast->declarations) {
        if (decl->type == ASTNodeType::SHADER_DECL) {
            auto* shader = static_cast<const ShaderDeclNode*>(decl.get());
            count += shader->statements.size();
        }
    }
    
    return count;
}

double ShaderCompiler::getCurrentTimeMs() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double, std::milli>(duration).count();
}