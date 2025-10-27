#pragma once

#include "parser.h"
#include <memory>
#include <set>
#include <map>
#include <string>

/**
 * Shader optimizer
 * Performs optimization passes on the AST before code generation
 */
class Optimizer {
public:
    Optimizer();
    
    /**
     * Optimize the AST
     * @param ast The AST to optimize (will be modified in place)
     */
    void optimize(ProgramNode* ast);
    
    /**
     * Get optimization statistics
     */
    struct OptimizationStats {
        int constantsFolded = 0;
        int deadCodeRemoved = 0;
        int algebraicSimplifications = 0;
        int totalPasses = 0;
    };
    
    const OptimizationStats& getStats() const { return stats; }
    
private:
    OptimizationStats stats;
    
    // Optimization pass methods
    bool constantFoldingPass(ASTNode* node);
    bool deadCodeEliminationPass(ProgramNode* ast);
    bool algebraicSimplificationPass(ASTNode* node);
    
    // Helper methods for constant folding
    bool isLiteral(ASTNode* node);
    bool isLiteralValue(ASTNode* node, float value);
    float getLiteralValue(ASTNode* node);
    std::unique_ptr<ASTNode> foldBinaryOp(const std::string& op, float left, float right);
    
    // Helper methods for dead code elimination
    void collectDeclaredVariables(ShaderDeclNode* shader, std::set<std::string>& declared);
    void collectUsedVariables(ASTNode* node, std::set<std::string>& used);
    void collectUsedVariablesInStatements(const std::vector<std::unique_ptr<ASTNode>>& statements, 
                                         std::set<std::string>& used);
    bool isOutputVariable(const std::string& varName, ShaderDeclNode* shader);
    
    // Helper methods for algebraic simplification
    std::unique_ptr<ASTNode> simplifyBinaryOp(BinaryOpNode* node, bool& changed);
    std::unique_ptr<ASTNode> cloneNode(ASTNode* node);
    
    // Traversal helpers
    bool optimizeNodeRecursive(ASTNode* node, bool (Optimizer::*optimizeFunc)(ASTNode*));
};