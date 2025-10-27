#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

// Forward declarations
struct Token;
struct ProgramNode;
class Lexer;
class Parser;
class Optimizer;
class CodeGenerator;

/**
 * Main shader compiler class
 * Orchestrates the compilation pipeline: Lexing -> Parsing -> Optimization -> Code Generation
 */
class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();
    
    /**
     * Compile a shader from source code to SPIR-V
     * @param source The shader source in custom DSL format
     * @param shaderType "vertex" or "fragment"
     * @return SPIR-V bytecode as uint32_t vector
     * @throws std::runtime_error on compilation failure
     */
    std::vector<uint32_t> compile(const std::string& source, const std::string& shaderType);
    
    /**
     * Compile from file
     * @param filename Path to shader source file
     * @param shaderType "vertex" or "fragment"
     * @return SPIR-V bytecode
     * @throws std::runtime_error if file cannot be opened or compilation fails
     */
    std::vector<uint32_t> compileFromFile(const std::string& filename, const std::string& shaderType);
    
    /**
     * Enable/disable optimization passes
     * @param enabled If true, optimization passes will be run
     */
    void setOptimizationEnabled(bool enabled) { optimizationEnabled = enabled; }
    
    /**
     * Check if optimization is enabled
     */
    bool isOptimizationEnabled() const { return optimizationEnabled; }
    
    /**
     * Enable/disable verbose output for debugging
     */
    void setVerbose(bool enabled) { verbose = enabled; }
    
    /**
     * Check if verbose mode is enabled
     */
    bool isVerbose() const { return verbose; }
    
    /**
     * Get compilation statistics (for debugging/metrics)
     */
    struct CompilationStats {
        size_t tokenCount = 0;
        size_t astNodeCount = 0;
        size_t originalStatementCount = 0;
        size_t optimizedStatementCount = 0;
        size_t constantsFolded = 0;
        size_t deadCodeEliminated = 0;
        size_t algebraicSimplifications = 0;
        size_t optimizationPasses = 0;
        size_t spirvSizeBytes = 0;
        size_t spirvInstructionCount = 0;
        double lexingTimeMs = 0.0;
        double parsingTimeMs = 0.0;
        double optimizationTimeMs = 0.0;
        double codegenTimeMs = 0.0;
        double totalTimeMs = 0.0;
    };
    
    const CompilationStats& getStats() const { return stats; }
    
    /**
     * Get the generated GLSL code (after optimization, before SPIR-V)
     * Only available after a successful compilation
     */
    const std::string& getGeneratedGLSL() const { return generatedGLSL; }
    
    /**
     * Reset statistics
     */
    void resetStats();
    
    /**
     * Validate shader type string
     */
    static bool isValidShaderType(const std::string& shaderType);
    
private:
    bool optimizationEnabled = true;
    bool verbose = false;
    CompilationStats stats;
    std::string generatedGLSL;
    
    // Helper methods
    void logVerbose(const std::string& message);
    void validateShaderType(const std::string& shaderType);
    size_t countASTNodes(const ProgramNode* ast);
    size_t countStatements(const ProgramNode* ast);
    double getCurrentTimeMs();
};

/**
 * Custom exception for shader compilation errors
 */
class ShaderCompilationError : public std::runtime_error {
public:
    enum class Stage {
        LEXING,
        PARSING,
        OPTIMIZATION,
        CODE_GENERATION
    };
    
    ShaderCompilationError(Stage stage, const std::string& message)
        : std::runtime_error(formatMessage(stage, message))
        , stage_(stage) {}
    
    Stage getStage() const { return stage_; }
    
private:
    Stage stage_;
    
    static std::string formatMessage(Stage stage, const std::string& message) {
        std::string stageStr;
        switch (stage) {
            case Stage::LEXING: stageStr = "Lexing"; break;
            case Stage::PARSING: stageStr = "Parsing"; break;
            case Stage::OPTIMIZATION: stageStr = "Optimization"; break;
            case Stage::CODE_GENERATION: stageStr = "Code Generation"; break;
        }
        return "[" + stageStr + " Error] " + message;
    }
};