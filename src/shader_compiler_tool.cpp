#include "shader_compiler.h"
#include <iostream>
#include <fstream>
#include <cstring>

void printUsage(const char* programName) {
    std::cout << "Vulkan Shader Compiler - Custom DSL to SPIR-V\n" << std::endl;
    std::cout << "Usage: " << programName << " <input.dsl> -o <output.spv> -t <vertex|fragment> [options]\n";
    std::cout << "\nRequired Arguments:\n";
    std::cout << "  <input.dsl>     Input shader file in custom DSL format\n";
    std::cout << "  -o <file>       Output SPIR-V file\n";
    std::cout << "  -t <type>       Shader type: 'vertex' or 'fragment'\n";
    std::cout << "\nOptional Arguments:\n";
    std::cout << "  --no-opt        Disable optimization passes\n";
    std::cout << "  --stats         Show detailed compilation statistics\n";
    std::cout << "  --verbose       Enable verbose compilation output\n";
    std::cout << "  --glsl          Output generated GLSL to stdout (for debugging)\n";
    std::cout << "  --help, -h      Show this help message\n";
    std::cout << "\nExamples:\n";
    std::cout << "  # Compile vertex shader with optimizations\n";
    std::cout << "  " << programName << " shader.vert.dsl -o shader.vert.spv -t vertex\n\n";
    std::cout << "  # Compile fragment shader without optimizations\n";
    std::cout << "  " << programName << " shader.frag.dsl -o shader.frag.spv -t fragment --no-opt\n\n";
    std::cout << "  # Compile with detailed statistics\n";
    std::cout << "  " << programName << " shader.vert.dsl -o shader.vert.spv -t vertex --stats --verbose\n";
}

int main(int argc, char* argv[]) {
    // Show help if requested or no arguments
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printUsage(argv[0]);
        return argc < 2 ? 1 : 0;
    }
    
    if (argc < 6) {
        std::cerr << "Error: Missing required arguments\n" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    std::string inputFile;
    std::string outputFile;
    std::string shaderType;
    bool enableOpt = true;
    bool showStats = false;
    bool verbose = false;
    bool showGLSL = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            shaderType = argv[++i];
        } else if (strcmp(argv[i], "--no-opt") == 0) {
            enableOpt = false;
        } else if (strcmp(argv[i], "--stats") == 0) {
            showStats = true;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "--glsl") == 0) {
            showGLSL = true;
        } else if (inputFile.empty() && argv[i][0] != '-') {
            inputFile = argv[i];
        }
    }
    
    // Validate required arguments
    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    if (outputFile.empty()) {
        std::cerr << "Error: No output file specified (use -o)\n" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    if (shaderType.empty()) {
        std::cerr << "Error: No shader type specified (use -t vertex or -t fragment)\n" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    if (!ShaderCompiler::isValidShaderType(shaderType)) {
        std::cerr << "Error: Invalid shader type '" << shaderType << "'" << std::endl;
        std::cerr << "Must be 'vertex' or 'fragment'\n" << std::endl;
        return 1;
    }
    
    try {
        std::cout << "=== Vulkan Shader Compiler ===" << std::endl;
        std::cout << "Input:  " << inputFile << std::endl;
        std::cout << "Output: " << outputFile << std::endl;
        std::cout << "Type:   " << shaderType << std::endl;
        std::cout << "Optimization: " << (enableOpt ? "enabled" : "disabled") << std::endl;
        std::cout << "==============================\n" << std::endl;
        
        // Create compiler instance
        ShaderCompiler compiler;
        compiler.setOptimizationEnabled(enableOpt);
        compiler.setVerbose(verbose);
        
        // Compile shader
        std::cout << "Compiling..." << std::endl;
        auto spirv = compiler.compileFromFile(inputFile, shaderType);
        
        // Write SPIR-V to output file
        std::ofstream outFile(outputFile, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Error: Failed to open output file: " << outputFile << std::endl;
            return 1;
        }
        
        outFile.write(reinterpret_cast<const char*>(spirv.data()), spirv.size() * sizeof(uint32_t));
        outFile.close();
        
        std::cout << "\n=== Compilation Successful ===" << std::endl;
        std::cout << "Output written to: " << outputFile << std::endl;
        std::cout << "SPIR-V size: " << spirv.size() * sizeof(uint32_t) << " bytes "
                  << "(" << spirv.size() << " words)" << std::endl;
        
        // Show generated GLSL if requested
        if (showGLSL) {
            std::cout << "\n=== Generated GLSL ===" << std::endl;
            std::cout << compiler.getGeneratedGLSL() << std::endl;
            std::cout << "======================" << std::endl;
        }
        
        // Show statistics if requested
        if (showStats) {
            auto stats = compiler.getStats();
            
            std::cout << "\n=== Compilation Statistics ===" << std::endl;
            
            // Timing
            std::cout << "\nTiming:" << std::endl;
            std::cout << "  Total:        " << stats.totalTimeMs << " ms" << std::endl;
            std::cout << "  Lexing:       " << stats.lexingTimeMs << " ms" << std::endl;
            std::cout << "  Parsing:      " << stats.parsingTimeMs << " ms" << std::endl;
            std::cout << "  Optimization: " << stats.optimizationTimeMs << " ms" << std::endl;
            std::cout << "  Code Gen:     " << stats.codegenTimeMs << " ms" << std::endl;
            
            // Lexer stats
            std::cout << "\nLexer:" << std::endl;
            std::cout << "  Tokens: " << stats.tokenCount << std::endl;
            
            // Parser stats
            std::cout << "\nParser:" << std::endl;
            std::cout << "  AST Nodes: " << stats.astNodeCount << std::endl;
            std::cout << "  Statements: " << stats.originalStatementCount << std::endl;
            
            // Optimization stats
            if (enableOpt) {
                std::cout << "\nOptimizer:" << std::endl;
                std::cout << "  Passes: " << stats.optimizationPasses << std::endl;
                std::cout << "  Constants folded: " << stats.constantsFolded << std::endl;
                std::cout << "  Algebraic simplifications: " << stats.algebraicSimplifications << std::endl;
                std::cout << "  Dead code eliminated: " << stats.deadCodeEliminated << std::endl;
                std::cout << "  Statements: " << stats.originalStatementCount 
                          << " -> " << stats.optimizedStatementCount;
                
                if (stats.originalStatementCount > 0) {
                    int reduction = stats.originalStatementCount - stats.optimizedStatementCount;
                    if (reduction > 0) {
                        float percent = 100.0f * reduction / stats.originalStatementCount;
                        std::cout << " (" << reduction << " removed, " 
                                  << percent << "% reduction)";
                    }
                }
                std::cout << std::endl;
            }
            
            // Code generator stats
            std::cout << "\nCode Generator:" << std::endl;
            std::cout << "  SPIR-V size: " << stats.spirvSizeBytes << " bytes" << std::endl;
            std::cout << "  SPIR-V instructions: " << stats.spirvInstructionCount << " words" << std::endl;
            
            std::cout << "==============================" << std::endl;
        }
        
        std::cout << "\nSuccess! You can now use this SPIR-V with Vulkan." << std::endl;
        
        return 0;
        
    } catch (const ShaderCompilationError& e) {
        std::cerr << "\n=== Compilation Failed ===" << std::endl;
        std::cerr << e.what() << std::endl;
        std::cerr << "==========================\n" << std::endl;
        return 1;
        
    } catch (const std::exception& e) {
        std::cerr << "\n=== Error ===" << std::endl;
        std::cerr << e.what() << std::endl;
        std::cerr << "=============\n" << std::endl;
        return 1;
    }
}