#include "lexer.h"
#include <cctype>
#include <stdexcept>
#include <map>

// Keyword map
static const std::map<std::string, TokenType> keywords = {
    {"shader", TokenType::SHADER},
    {"vertex", TokenType::VERTEX},
    {"fragment", TokenType::FRAGMENT},
    {"input", TokenType::INPUT},
    {"output", TokenType::OUTPUT},
    {"uniform", TokenType::UNIFORM},
    {"main", TokenType::MAIN},
    {"vec2", TokenType::VEC2},
    {"vec3", TokenType::VEC3},
    {"vec4", TokenType::VEC4},
    {"mat4", TokenType::MAT4},
    {"float", TokenType::FLOAT},
    {"int", TokenType::INT}
};

Lexer::Lexer(const std::string& src) : source(src) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    while (position < source.length()) {
        skipWhitespace();
        
        if (position >= source.length()) {
            break;
        }
        
        char c = currentChar();
        
        // Skip comments
        if (c == '/' && peek() == '/') {
            skipComment();
            continue;
        }
        
        // Numbers
        if (std::isdigit(c) || (c == '.' && std::isdigit(peek()))) {
            tokens.push_back(readNumber());
            continue;
        }
        
        // Identifiers and keywords
        if (std::isalpha(c) || c == '_') {
            tokens.push_back(readIdentifier());
            continue;
        }
        
        // Single character tokens
        Token token;
        token.line = line;
        token.column = column;
        
        switch (c) {
            case '+': token = makeToken(TokenType::PLUS, "+"); break;
            case '-': token = makeToken(TokenType::MINUS, "-"); break;
            case '*': token = makeToken(TokenType::MULTIPLY, "*"); break;
            case '/': token = makeToken(TokenType::DIVIDE, "/"); break;
            case '=': token = makeToken(TokenType::ASSIGN, "="); break;
            case '(': token = makeToken(TokenType::LPAREN, "("); break;
            case ')': token = makeToken(TokenType::RPAREN, ")"); break;
            case '{': token = makeToken(TokenType::LBRACE, "{"); break;
            case '}': token = makeToken(TokenType::RBRACE, "}"); break;
            case ';': token = makeToken(TokenType::SEMICOLON, ";"); break;
            case ',': token = makeToken(TokenType::COMMA, ","); break;
            case '.': token = makeToken(TokenType::DOT, "."); break;
            default:
                throw std::runtime_error("Unexpected character: " + std::string(1, c));
        }
        
        tokens.push_back(token);
        advance();
    }
    
    Token eof;
    eof.type = TokenType::END_OF_FILE;
    eof.value = "";
    eof.line = line;
    eof.column = column;
    tokens.push_back(eof);
    
    return tokens;
}

char Lexer::currentChar() {
    if (position >= source.length()) {
        return '\0';
    }
    return source[position];
}

char Lexer::peek(int offset) {
    if (position + offset >= source.length()) {
        return '\0';
    }
    return source[position + offset];
}

void Lexer::advance() {
    if (position < source.length()) {
        if (source[position] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        position++;
    }
}

void Lexer::skipWhitespace() {
    while (position < source.length() && std::isspace(currentChar())) {
        advance();
    }
}

void Lexer::skipComment() {
    // Skip //
    advance();
    advance();
    
    // Skip until end of line
    while (currentChar() != '\n' && currentChar() != '\0') {
        advance();
    }
}

Token Lexer::readNumber() {
    Token token;
    token.line = line;
    token.column = column;
    token.type = TokenType::NUMBER;
    
    std::string value;
    bool hasDecimal = false;
    
    while (std::isdigit(currentChar()) || (currentChar() == '.' && !hasDecimal)) {
        if (currentChar() == '.') {
            hasDecimal = true;
        }
        value += currentChar();
        advance();
    }
    
    token.value = value;
    return token;
}

Token Lexer::readIdentifier() {
    Token token;
    token.line = line;
    token.column = column;
    
    std::string value;
    while (std::isalnum(currentChar()) || currentChar() == '_') {
        value += currentChar();
        advance();
    }
    
    // Check if it's a keyword
    auto it = keywords.find(value);
    if (it != keywords.end()) {
        token.type = it->second;
    } else {
        token.type = TokenType::IDENTIFIER;
    }
    
    token.value = value;
    return token;
}

Token Lexer::makeToken(TokenType type, const std::string& value) {
    Token token;
    token.type = type;
    token.value = value;
    token.line = line;
    token.column = column;
    return token;
}