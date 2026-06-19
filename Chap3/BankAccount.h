#ifndef BANKACCOUNT_H_INCLUDED
#define BANKACCOUNT_H_INCLUDED

#include <string>
#include <vector>

class Transaction
{
public:
    int id;
    double amount;
    char* description;

    Transaction(int i, double a, const char* desc);
    ~Transaction();
};

class BankAccount
{
private:
    int accountId;
    std::string owner;
    double* balance;
    std::vector<Transaction*> transactions;

public:
    BankAccount(int id, std::string name, double initialBalance);
    ~BankAccount();

    void deposit(double amount);
    void withdraw(double amount);
    void transfer(BankAccount& target, double amount);

    double getBalance() const;

    Transaction* getTransaction(int index);

    void addTransaction(Transaction* t);

    void printStatement();

    char* generateReport();
};

#endif // BANKACCOUNT_H_INCLUDED
