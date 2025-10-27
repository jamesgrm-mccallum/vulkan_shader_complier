#include "codegen.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

int CodeGenerator::tempFileCounter = 0;

CodeGenerator::CodeGenerator() {}

std::vector<uint32_t> CodeGenerator::generate(const ProgramNode* ast, const std::string& shaderType) {
    // Step 1: Generate GLSL code from AST
    std::string glslCode = generateGLSL(ast, shaderType);
    lastGeneratedGLSL = glslCode;
    
    // Step 2: Compile GLSL to SPIR-V
    std::vector<uint32_t> spirv = compileGLSLToSPIRV(glslCode, shaderType);
    
    return spirv;
}

std::string CodeGenerator::generateGLSL(const ProgramNode* ast, const std::string& shaderType) {
    std::stringstream ss;
    
    // GLSL version header
    ss << "#version 450\n\n";
    
    // Reset location counters
    nextInputLocation = 0;
    nextOutputLocation = 0;
    inputLocations.clear();
    outputLocations.clear();
    
    // Find the shader declaration matching the requested type
    const ShaderDeclNode* targetShader = nullptr;
    for (const auto& decl : ast->declarations) {
        if (decl->type == ASTNodeType::SHADER_DECL) {
            auto* shader = static_cast<const ShaderDeclNode*>(decl.get());
            if (shader->shaderType == shaderType) {
                targetShader = shader;
                break;
            }
        }
    }
    
    if (!targetShader) {
        throw std::runtime_error("No shader declaration found for type: " + shaderType);
    }
    
    // Generate shader code
    ss << generateShaderDeclaration(targetShader);
    
    return ss.str();
}

std::string CodeGenerator::generateShaderDeclaration(const ShaderDeclNode* shader) {
    std::stringstream ss;
    
    // Generate input declarations
    ss << generateInputDeclarations(shader->inputs);
    
    // Generate output declarations
    ss << generateOutputDeclarations(shader->outputs);
    
    // Generate main function
    ss << generateMainFunction(shader->statements);
    
    return ss.str();
}

std::string CodeGenerator::generateInputDeclarations(const std::vector<std::unique_ptr<ASTNode>>& inputs) {
    std::stringstream ss;
    
    for (const auto& input : inputs) {
        if (input->type == ASTNodeType::VARIABLE_DECL) {
            auto* varDecl = static_cast<const VariableDeclNode*>(input.get());
            
            // Assign and track location
            int location = nextInputLocation++;
            inputLocations[varDecl->name] = location;
            
            ss << "layout(location = " << location << ") in " 
               << mapType(varDecl->varType) << " " 
               << varDecl->name << ";\n";
        }
    }
    
    if (!inputs.empty()) {
        ss << "\n";
    }
    
    return ss.str();
}

std::string CodeGenerator::generateOutputDeclarations(const std::vector<std::unique_ptr<ASTNode>>& outputs) {
    std::stringstream ss;
    
    for (const auto& output : outputs) {
        if (output->type == ASTNodeType::VARIABLE_DECL) {
            auto* varDecl = static_cast<const VariableDeclNode*>(output.get());
            
            // Assign and track location
            int location = nextOutputLocation++;
            outputLocations[varDecl->name] = location;
            
            ss << "layout(location = " << location << ") out " 
               << mapType(varDecl->varType) << " " 
               << varDecl->name << ";\n";
        }
    }
    
    if (!outputs.empty()) {
        ss << "\n";
    }
    
    return ss.str();
}

std::string CodeGenerator::generateMainFunction(const std::vector<std::unique_ptr<ASTNode>>& statements) {
    std::stringstream ss;
    
    ss << "void main() {\n";
    
    for (const auto& stmt : statements) {
        ss << "    " << generateStatement(stmt.get()) << "\n";
    }
    
    ss << "}\n";
    
    return ss.str();
}

std::string CodeGenerator::generateStatement(const ASTNode* node) {
    if (!node) {
        return "";
    }
    
    switch (node->type) {
        case ASTNodeType::ASSIGNMENT: {
            auto* assign = static_cast<const AssignmentNode*>(node);
            return generateExpression(assign->target.get()) + " = " + 
                   generateExpression(assign->value.get()) + ";";
        }
        
        default:
            throw std::runtime_error("Unsupported statement type in code generation");
    }
}

std::string CodeGenerator::generateExpression(const ASTNode* node) {
    if (!node) {
        return "";
    }
    
    switch (node->type) {
        case ASTNodeType::BINARY_OP: {
            auto* binOp = static_cast<const BinaryOpNode*>(node);
            return "(" + generateExpression(binOp->left.get()) + " " + 
                   binOp->op + " " + 
                   generateExpression(binOp->right.get()) + ")";
        }
        
        case ASTNodeType::IDENTIFIER: {
            auto* id = static_cast<const IdentifierNode*>(node);
            return id->name;
        }
        
        case ASTNodeType::LITERAL: {
            auto* lit = static_cast<const LiteralNode*>(node);
            return lit->value;
        }
        
        case ASTNodeType::MEMBER_ACCESS: {
            auto* member = static_cast<const MemberAccessNode*>(node);
            return generateExpression(member->object.get()) + "." + member->member;
        }
        
        case ASTNodeType::FUNCTION_CALL: {
            auto* funcCall = static_cast<const FunctionCallNode*>(node);
            std::stringstream ss;
            ss << funcCall->functionName << "(";
            
            for (size_t i = 0; i < funcCall->arguments.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << generateExpression(funcCall->arguments[i].get());
            }
            
            ss << ")";
            return ss.str();
        }
        
        default:
            throw std::runtime_error("Unsupported expression type in code generation");
    }
}

std::vector<uint32_t> CodeGenerator::compileGLSLToSPIRV(const std::string& glslCode, 
                                                         const std::string& shaderType) {
    // Generate unique temporary file paths
    std::string inputFile = generateTempFilePath(getFileExtension(shaderType));
    std::string outputFile = generateTempFilePath("spv");
    
    try {
        // Step 1: Write GLSL to temporary file
        std::ofstream outFile(inputFile);
        if (!outFile.is_open()) {
            throw std::runtime_error("Failed to create temporary GLSL file: " + inputFile);
        }
        outFile << glslCode;
        outFile.close();
        
        // Step 2: Build glslangValidator command
        std::stringstream cmd;
        cmd << "glslangValidator -V " << inputFile << " -o " << outputFile << " 2>&1";
        
        // Step 3: Execute glslangValidator
        FILE* pipe = popen(cmd.str().c_str(), "r");
        if (!pipe) {
            throw std::runtime_error("Failed to execute glslangValidator");
        }
        
        // Capture output
        std::stringstream output;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output << buffer;
        }
        
        int result = pclose(pipe);
        
        if (result != 0) {
            std::string errorMsg = "GLSL compilation failed!\n";
            errorMsg += "Command: " + cmd.str() + "\n";
            errorMsg += "Output: " + output.str() + "\n";
            errorMsg += "Generated GLSL:\n" + glslCode;
            
            // Clean up before throwing
            cleanupTempFile(inputFile);
            cleanupTempFile(outputFile);
            
            throw std::runtime_error(errorMsg);
        }
        
        // Step 4: Read SPIR-V file
        if (!fileExists(outputFile)) {
            cleanupTempFile(inputFile);
            throw std::runtime_error("SPIR-V output file was not created: " + outputFile);
        }
        
        std::vector<uint32_t> spirv = readSPIRVFile(outputFile);
        
        // Step 5: Clean up temporary files
        cleanupTempFile(inputFile);
        cleanupTempFile(outputFile);
        
        return spirv;
        
    } catch (...) {
        // Ensure cleanup on any exception
        cleanupTempFile(inputFile);
        cleanupTempFile(outputFile);
        throw;
    }
}

std::vector<uint32_t> CodeGenerator::readSPIRVFile(const std::string& filename) {
    // Open file in binary mode
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open SPIR-V file: " + filename);
    }
    
    // Get file size
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Validate file size
    if (fileSize % sizeof(uint32_t) != 0) {
        throw std::runtime_error("Invalid SPIR-V file: size is not a multiple of 4 bytes");
    }
    
    // Read into uint32_t vector
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        throw std::runtime_error("Failed to read SPIR-V file: " + filename);
    }
    
    file.close();
    
    // Validate SPIR-V magic number
    if (!buffer.empty() && buffer[0] != 0x07230203) {
        throw std::runtime_error("Invalid SPIR-V file: incorrect magic number");
    }
    
    return buffer;
}

// Helper methods

std::string CodeGenerator::getFileExtension(const std::string& shaderType) {
    if (shaderType == "vertex") {
        return "vert";
    } else if (shaderType == "fragment") {
        return "frag";
    } else {
        throw std::runtime_error("Unknown shader type: " + shaderType);
    }
}

std::string CodeGenerator::generateTempFilePath(const std::string& extension) {
    std::stringstream ss;
    ss << "/tmp/shader_" << getpid() << "_" << tempFileCounter++ << "." << extension;
    return ss.str();
}

void CodeGenerator::cleanupTempFile(const std::string& filename) {
    if (fileExists(filename)) {
        std::remove(filename.c_str());
    }
}

bool CodeGenerator::fileExists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

std::string CodeGenerator::mapType(const std::string& type) {
    // Direct mapping - GLSL types match our DSL types
    if (type == "vec2" || type == "vec3" || type == "vec4" ||
        type == "mat4" || type == "float" || type == "int") {
        return type;
    }
    
    // If no mapping found, return as-is
    return type;
}