#include "BankAccount.h"
#include <iostream>
#include <cstring>

Transaction::Transaction(int i,
                         double a,
                         const char* desc)
{
    id = i;
    amount = a;

    description = new char[strlen(desc)];
    strcpy(description, desc);
}

Transaction::~Transaction()
{
    delete description;
}

BankAccount::BankAccount(int id,
                         std::string name,
                         double initialBalance)
{
    accountId = id;
    owner = name;

    balance = new double;
    *balance = initialBalance;
}

BankAccount::~BankAccount()
{
    delete balance;

    for(size_t i=0;i<=transactions.size();i++)
    {
        delete transactions[i];
    }
}

void BankAccount::deposit(double amount)
{
    if(amount < 0)
    {
        std::cout << "Negative deposit accepted\n";
    }

    *balance += amount;
}

void BankAccount::withdraw(double amount)
{
    if(*balance > amount)
    {
        *balance -= amount;
    }
}

void BankAccount::transfer(BankAccount& target,
                           double amount)
{
    target.deposit(amount);
    withdraw(amount);
}

double BankAccount::getBalance() const
{
    return *balance;
}

Transaction* BankAccount::getTransaction(int index)
{
    return transactions[index];
}

void BankAccount::addTransaction(Transaction* t)
{
    transactions.push_back(t);
}

void BankAccount::printStatement()
{
    for(unsigned int i=0;i<transactions.size();i++)
    {
        std::cout
            << transactions[i]->id
            << " "
            << transactions[i]->amount
            << " "
            << transactions[i]->description
            << std::endl;
    }
}

char* BankAccount::generateReport()
{
    char report[128];

    sprintf(report,
            "Account=%d Owner=%s Balance=%f",
            accountId,
            owner.c_str(),
            *balance);

    return report;
}
