#include <iostream>
#include <cassert>
#include <string>
#include <memory>
#include <functional>

#include "../src/core/ast/ast.h"
#include "../src/core/evaluator/evaluator.h"
#include "../src/core/generator/markdown_formatter.h"

using namespace madola;

// Test framework
class TestSuite {
private:
    int passed = 0;
    int failed = 0;
    std::string current_test;

public:
    void run_test(const std::string& name, std::function<void()> test) {
        current_test = name;
        std::cout << "Running: " << name << "... ";
        try {
            test();
            std::cout << "PASS" << std::endl;
            passed++;
        } catch (const std::exception& e) {
            std::cout << "FAIL - " << e.what() << std::endl;
            failed++;
        }
    }

    void assert_equal(const std::string& expected, const std::string& actual) {
        if (expected != actual) {
            throw std::runtime_error("Expected '" + expected + "' but got '" + actual + "'");
        }
    }

    void assert_true(bool condition, const std::string& message = "Assertion failed") {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    void summary() {
        std::cout << "\n=== Test Results ===" << std::endl;
        std::cout << "Passed: " << passed << std::endl;
        std::cout << "Failed: " << failed << std::endl;
        std::cout << "Total:  " << (passed + failed) << std::endl;

        if (failed == 0) {
            std::cout << "🎉 All tests passed!" << std::endl;
        } else {
            std::cout << "❌ " << failed << " test(s) failed" << std::endl;
        }
    }

    int get_exit_code() { return failed > 0 ? 1 : 0; }
};

// Test cases
void test_ast_creation(TestSuite& suite) {
    suite.run_test("AST Creation", []() {
        auto program = std::make_unique<Program>();

        auto number = std::make_unique<Number>(42);
        auto assignment = std::make_unique<AssignmentStatement>("x", std::move(number));
        program->addStatement(std::move(assignment));

        if (program->statements.size() != 1) {
            throw std::runtime_error("Expected 1 statement");
        }
    });
}

void test_number_evaluation(TestSuite& suite) {
    suite.run_test("Number Evaluation", [&suite]() {
        auto program = std::make_unique<Program>();
        program->addStatement(std::make_unique<VersionStatement>("0.01"));
        auto number = std::make_unique<Number>(123);
        auto print_stmt = std::make_unique<PrintStatement>(std::move(number));
        program->addStatement(std::move(print_stmt));

        Evaluator evaluator;
        auto result = evaluator.evaluate(*program);

        suite.assert_true(result.success, "Evaluation should succeed");
        suite.assert_equal("123", result.outputs[0]);
    });
}

void test_variable_assignment_and_retrieval(TestSuite& suite) {
    suite.run_test("Variable Assignment and Retrieval", [&suite]() {
        auto program = std::make_unique<Program>();
        program->addStatement(std::make_unique<VersionStatement>("0.01"));

        // x := 99;
        auto number = std::make_unique<Number>(99);
        auto assignment = std::make_unique<AssignmentStatement>("x", std::move(number));
        program->addStatement(std::move(assignment));

        // print(x);
        auto identifier = std::make_unique<Identifier>("x");
        auto print_stmt = std::make_unique<PrintStatement>(std::move(identifier));
        program->addStatement(std::move(print_stmt));

        Evaluator evaluator;
        auto result = evaluator.evaluate(*program);

        suite.assert_true(result.success, "Evaluation should succeed");
        suite.assert_equal("99", result.outputs[0]);
    });
}

void test_multiple_assignments(TestSuite& suite) {
    suite.run_test("Multiple Assignments", [&suite]() {
        auto program = std::make_unique<Program>();
        program->addStatement(std::make_unique<VersionStatement>("0.01"));

        // x := 10;
        auto num1 = std::make_unique<Number>(10);
        auto assign1 = std::make_unique<AssignmentStatement>("x", std::move(num1));
        program->addStatement(std::move(assign1));

        // y := 20;
        auto num2 = std::make_unique<Number>(20);
        auto assign2 = std::make_unique<AssignmentStatement>("y", std::move(num2));
        program->addStatement(std::move(assign2));

        // print(x); print(y);
        auto id1 = std::make_unique<Identifier>("x");
        auto print1 = std::make_unique<PrintStatement>(std::move(id1));
        program->addStatement(std::move(print1));

        auto id2 = std::make_unique<Identifier>("y");
        auto print2 = std::make_unique<PrintStatement>(std::move(id2));
        program->addStatement(std::move(print2));

        Evaluator evaluator;
        auto result = evaluator.evaluate(*program);

        suite.assert_true(result.success, "Evaluation should succeed");
        suite.assert_equal("10", result.outputs[0]);
        suite.assert_equal("20", result.outputs[1]);
    });
}

void test_undefined_variable_error(TestSuite& suite) {
    suite.run_test("Undefined Variable Error", [&suite]() {
        auto program = std::make_unique<Program>();
        program->addStatement(std::make_unique<VersionStatement>("0.01"));

        // print(undefined_var);
        auto identifier = std::make_unique<Identifier>("undefined_var");
        auto print_stmt = std::make_unique<PrintStatement>(std::move(identifier));
        program->addStatement(std::move(print_stmt));

        Evaluator evaluator;
        auto result = evaluator.evaluate(*program);

        suite.assert_true(!result.success, "Evaluation should fail");
        suite.assert_true(result.error.find("Undefined variable") != std::string::npos,
                         "Should contain undefined variable error");
    });
}

void test_markdown_formatting(TestSuite& suite) {
    suite.run_test("Markdown Formatting", [&suite]() {
        auto program = std::make_unique<Program>();

        auto number = std::make_unique<Number>(42);
        auto assignment = std::make_unique<AssignmentStatement>("test", std::move(number));
        program->addStatement(std::move(assignment));

        MarkdownFormatter formatter;
        auto result = formatter.formatProgram(*program);

        suite.assert_true(result.success, "Formatting should succeed");
        suite.assert_true(result.markdown.find("```madola") != std::string::npos,
                         "Should contain madola code block");
        suite.assert_true(result.markdown.find("test =42;") != std::string::npos,
                         "Should contain the assignment, got: " + result.markdown);
    });
}

void test_markdown_with_execution(TestSuite& suite) {
    suite.run_test("Markdown with Execution", [&suite]() {
        auto program = std::make_unique<Program>();
        program->addStatement(std::make_unique<VersionStatement>("0.01"));

        auto number = std::make_unique<Number>(777);
        auto print_stmt = std::make_unique<PrintStatement>(std::move(number));
        program->addStatement(std::move(print_stmt));

        MarkdownFormatter formatter;
        auto result = formatter.formatProgramWithExecution(*program);

        suite.assert_true(result.success, "Formatting should succeed");
        suite.assert_true(result.markdown.find("777") != std::string::npos,
                         "Should contain output value, got: " + result.markdown);
    });
}

void test_record_member_access(TestSuite& suite) {
    suite.run_test("Record Member Access", [&suite]() {
        auto program = std::make_unique<Program>();
        program->addStatement(std::make_unique<VersionStatement>("0.01"));

        // s := testrecord(1, 2);
        std::vector<ExpressionPtr> args;
        args.push_back(std::make_unique<Number>(1));
        args.push_back(std::make_unique<Number>(2));
        auto call = std::make_unique<FunctionCall>("testrecord", std::move(args));
        auto assignment = std::make_unique<AssignmentStatement>("s", std::move(call));
        program->addStatement(std::move(assignment));

        // print(s.x); print(s.y);
        auto sIdent1 = std::make_unique<Identifier>("s");
        auto memberX = std::make_unique<MemberAccess>(std::move(sIdent1), "x");
        auto printX = std::make_unique<PrintStatement>(std::move(memberX));
        program->addStatement(std::move(printX));

        auto sIdent2 = std::make_unique<Identifier>("s");
        auto memberY = std::make_unique<MemberAccess>(std::move(sIdent2), "y");
        auto printY = std::make_unique<PrintStatement>(std::move(memberY));
        program->addStatement(std::move(printY));

        Evaluator evaluator;
        auto result = evaluator.evaluate(*program);

        suite.assert_true(result.success, "Evaluation should succeed");
        suite.assert_equal("1", result.outputs[0]);
        suite.assert_equal("2", result.outputs[1]);
    });
}

void test_record_undefined_field_error(TestSuite& suite) {
    suite.run_test("Record Undefined Field Error", [&suite]() {
        auto program = std::make_unique<Program>();
        program->addStatement(std::make_unique<VersionStatement>("0.01"));

        std::vector<ExpressionPtr> args;
        args.push_back(std::make_unique<Number>(1));
        args.push_back(std::make_unique<Number>(2));
        auto call = std::make_unique<FunctionCall>("testrecord", std::move(args));
        auto assignment = std::make_unique<AssignmentStatement>("s", std::move(call));
        program->addStatement(std::move(assignment));

        auto sIdent = std::make_unique<Identifier>("s");
        auto memberZ = std::make_unique<MemberAccess>(std::move(sIdent), "z");
        auto printZ = std::make_unique<PrintStatement>(std::move(memberZ));
        program->addStatement(std::move(printZ));

        Evaluator evaluator;
        auto result = evaluator.evaluate(*program);

        suite.assert_true(!result.success, "Evaluation should fail for an undefined field");
        suite.assert_true(result.error.find("Undefined field") != std::string::npos,
                         "Error should mention the undefined field, got: " + result.error);
    });
}

int main() {
    std::cout << "MADOLA Language Test Suite" << std::endl;
    std::cout << "==========================" << std::endl;

    TestSuite suite;

    // Run all tests
    test_ast_creation(suite);
    test_number_evaluation(suite);
    test_variable_assignment_and_retrieval(suite);
    test_multiple_assignments(suite);
    test_undefined_variable_error(suite);
    test_markdown_formatting(suite);
    test_markdown_with_execution(suite);
    test_record_member_access(suite);
    test_record_undefined_field_error(suite);

    suite.summary();
    return suite.get_exit_code();
}