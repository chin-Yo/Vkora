#include <functional>
#include <iostream>
#include "Reflection/StaticRefl.hpp"

class father
{
    std::string name = "father";
};

class ExampleClass : public father
{
public:
    ExampleClass() : m_double(0.0054), m_constInt(42)
    {
    }

    ExampleClass(int i, double d, const std::string &s)
        : m_int(i), m_double(d), m_string(s), m_constInt(42)
    {
    }

    ~ExampleClass()
    {
        std::cout << "ExampleClass destructor called" << std::endl;
    }

    void displayInfo() const
    {
        std::cout << "Int: " << m_int << ", Double: " << m_double
                  << ", String: " << m_string << std::endl;
    }

    void processData(int value)
    {
        m_int += value;
    }

    void processData(double value)
    {
        m_double += value;
    }

    static int getStaticValue()
    {
        return s_staticValue;
    }

    inline const std::string &getString() const { return m_string; }

    template <typename T>
    void printValue(const T &value)
    {
        std::cout << "Value: " << value << std::endl;
    }

    void setCallback(std::function<void(int)> callback)
    {
        m_callback = callback;
    }

    void executeCallback(int value)
    {
        if (m_callback)
        {
            m_callback(value);
        }
    }

    friend void friendFunction(const ExampleClass &ec);

    int m_int = 434;
    double m_double;
    bool m_flag = false;
    char m_char = 'A';

    std::string m_string;
    std::vector<int> m_vector;

    const int m_constInt;

    static int s_staticValue;

    std::function<void(int)> m_callback;
};

REFL_INGO(ExampleClass)
VARIABLES(
    VAR(&ExampleClass::m_int), VAR(&ExampleClass::m_double), VAR(&ExampleClass::m_flag))
REFL_END()

int main()
{
    father *ec = new ExampleClass();

    auto info = ReflInfo<ExampleClass>();

    VisitVarTuple(info.Variables, ec);

    return 0;
}
