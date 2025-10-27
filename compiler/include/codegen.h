#pragma once

#include "parser.h"
#include <vector>
#include <string>
#include <map>
#include <sstream>

/**
 * Code generator
 * Converts AST to GLSL and then to SPIR-V
 */
class CodeGenerator {
public:
    CodeGenerator();
    
    /**
     * Generate SPIR-V from AST
     * @param ast The optimized AST
     * @param shaderType "vertex" or "fragment"
     * @return SPIR-V bytecode
     */
    std::vector<uint32_t> generate(const ProgramNode* ast, const std::string& shaderType);
    
    /**
     * Get the generated GLSL code (for debugging/inspection)
     */
    const std::string& getGeneratedGLSL() const { return lastGeneratedGLSL; }
    
private:
    // GLSL generation methods
    std::string generateGLSL(const ProgramNode* ast, const std::string& shaderType);
    std::string generateShaderDeclaration(const ShaderDeclNode* shader);
    std::string generateInputDeclarations(const std::vector<std::unique_ptr<ASTNode>>& inputs);
    std::string generateOutputDeclarations(const std::vector<std::unique_ptr<ASTNode>>& outputs);
    std::string generateMainFunction(const std::vector<std::unique_ptr<ASTNode>>& statements);
    std::string generateStatement(const ASTNode* node);
    std::string generateExpression(const ASTNode* node);
    
    // SPIR-V compilation methods
    std::vector<uint32_t> compileGLSLToSPIRV(const std::string& glslCode, const std::string& shaderType);
    std::vector<uint32_t> readSPIRVFile(const std::string& filename);
    
    // Helper methods
    std::string getFileExtension(const std::string& shaderType);
    std::string generateTempFilePath(const std::string& extension);
    void cleanupTempFile(const std::string& filename);
    bool fileExists(const std::string& filename);
    
    // Type mapping
    std::string mapType(const std::string& type);
    
    // Variable location tracking
    std::map<std::string, int> inputLocations;
    std::map<std::string, int> outputLocations;
    int nextInputLocation = 0;
    int nextOutputLocation = 0;
    
    // Store last generated GLSL for debugging
    std::string lastGeneratedGLSL;
    
    // Temp file counter for unique names
    static int tempFileCounter;
};