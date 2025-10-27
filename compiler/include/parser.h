#pragma once

#include "lexer.h"
#include <memory>
#include <vector>
#include <string>

/**
 * AST Node types
 */
enum class ASTNodeType {
    PROGRAM,
    SHADER_DECL,
    VARIABLE_DECL,
    FUNCTION_DECL,
    ASSIGNMENT,
    BINARY_OP,
    IDENTIFIER,
    LITERAL,
    MEMBER_ACCESS,
    FUNCTION_CALL
};

/**
 * Base AST Node
 */
struct ASTNode {
    ASTNodeType type;
    virtual ~ASTNode() = default;
};

/**
 * Program node (root of AST)
 */
struct ProgramNode : public ASTNode {
    std::vector<std::unique_ptr<ASTNode>> declarations;
    ProgramNode() { type = ASTNodeType::PROGRAM; }
};

/**
 * Shader declaration node
 */
struct ShaderDeclNode : public ASTNode {
    std::string shaderType; // "vertex" or "fragment"
    std::vector<std::unique_ptr<ASTNode>> inputs;
    std::vector<std::unique_ptr<ASTNode>> outputs;
    std::vector<std::unique_ptr<ASTNode>> statements;
    ShaderDeclNode() { type = ASTNodeType::SHADER_DECL; }
};

/**
 * Variable declaration node
 */
struct VariableDeclNode : public ASTNode {
    std::string varType; // vec3, float, etc.
    std::string name;
    VariableDeclNode() { type = ASTNodeType::VARIABLE_DECL; }
};

/**
 * Assignment node
 */
struct AssignmentNode : public ASTNode {
    std::unique_ptr<ASTNode> target;
    std::unique_ptr<ASTNode> value;
    AssignmentNode() { type = ASTNodeType::ASSIGNMENT; }
};

/**
 * Binary operation node
 */
struct BinaryOpNode : public ASTNode {
    std::string op; // +, -, *, /
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    BinaryOpNode() { type = ASTNodeType::BINARY_OP; }
};

/**
 * Identifier node
 */
struct IdentifierNode : public ASTNode {
    std::string name;
    IdentifierNode() { type = ASTNodeType::IDENTIFIER; }
};

/**
 * Literal node
 */
struct LiteralNode : public ASTNode {
    std::string value;
    LiteralNode() { type = ASTNodeType::LITERAL; }
};

/**
 * Member access node (e.g., position.xyz)
 */
struct MemberAccessNode : public ASTNode {
    std::unique_ptr<ASTNode> object;
    std::string member;
    MemberAccessNode() { type = ASTNodeType::MEMBER_ACCESS; }
};

/**
 * Function call node (e.g., vec4(position, 1.0))
 */
struct FunctionCallNode : public ASTNode {
    std::string functionName;
    std::vector<std::unique_ptr<ASTNode>> arguments;
    FunctionCallNode() { type = ASTNodeType::FUNCTION_CALL; }
};

/**
 * Parser for the shader language
 * Builds an Abstract Syntax Tree (AST) from tokens
 */
class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    
    /**
     * Parse tokens into AST
     * @return Root program node
     */
    std::unique_ptr<ProgramNode> parse();
    
private:
    std::vector<Token> tokens;
    size_t position = 0;
    
    // Token navigation helpers
    Token& current();
    Token& peek(int offset = 1);
    void advance();
    bool match(TokenType type);
    void expect(TokenType type, const std::string& message);
    bool check(TokenType type);
    
    // Parsing methods for different constructs
    std::unique_ptr<ShaderDeclNode> parseShaderDecl();
    std::unique_ptr<VariableDeclNode> parseVariableDecl(bool isInput);
    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<ASTNode> parseExpression();
    std::unique_ptr<ASTNode> parseTerm();
    std::unique_ptr<ASTNode> parseFactor();
    std::unique_ptr<ASTNode> parsePrimary();
    std::unique_ptr<FunctionCallNode> parseFunctionCall(const std::string& funcName);
    
    // Helper to parse type tokens
    std::string parseType();
    bool isTypeToken(TokenType type);
};