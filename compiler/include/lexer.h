#pragma once

#include <string>
#include <vector>

/**
 * Token types for the custom shader language
 */
enum class TokenType {
    // Keywords
    SHADER,
    VERTEX,
    FRAGMENT,
    INPUT,
    OUTPUT,
    UNIFORM,
    MAIN,
    
    // Types
    VEC2,
    VEC3,
    VEC4,
    MAT4,
    FLOAT,
    INT,
    
    // Identifiers and literals
    IDENTIFIER,
    NUMBER,
    
    // Operators
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    ASSIGN,
    
    // Delimiters
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    SEMICOLON,
    COMMA,
    DOT,
    
    // Special
    END_OF_FILE,
    UNKNOWN
};

/**
 * Token structure
 */
struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};

/**
 * Lexer (Tokenizer) for the shader language
 * Converts source code into a stream of tokens
 */
class Lexer {
public:
    Lexer(const std::string& source);
    
    /**
     * Tokenize the entire source
     * @return Vector of tokens
     */
    std::vector<Token> tokenize();
    
private:
    std::string source;
    size_t position = 0;
    int line = 1;
    int column = 1;
    
    char currentChar();
    char peek(int offset = 1);
    void advance();
    void skipWhitespace();
    void skipComment();
    
    Token readNumber();
    Token readIdentifier();
    Token makeToken(TokenType type, const std::string& value);
};