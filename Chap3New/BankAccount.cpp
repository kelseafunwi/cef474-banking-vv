#include "BankAccount.h"
#include <iostream>
#include <cstring>
#include <cstdio>

Transaction::Transaction(int i,
                         double a,
                         const char* desc)
{
    id = i;
    amount = a;

    description = new char[strlen(desc) + 1];
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

    for(size_t i=0;i<transactions.size();i++)
    {
        delete transactions[i];
    }
    transactions.clear();
}

void BankAccount::deposit(double amount)
{
    if(amount <= 0)
    {
        std::cerr << "Error: Deposit amount must be positive.\n";
        return;
    }

    *balance += amount;
}

void BankAccount::withdraw(double amount)
{
    if(amount <= 0)
    {
        std::cerr << "Error: Withdrawal amount must be positive.\n";
        return;
    }
    if(*balance < amount)
    {
        std::cerr << "Error: Insufficient funds.\n";
        return;
    }

    *balance -= amount;
}

void BankAccount::transfer(BankAccount& target,
                           double amount)
{
    if(amount <= 0)
    {
        std::cerr << "Error: Transfer amount must be positive.\n";
        return;
    }
    if(*balance < amount)
    {
        std::cerr << "Error: Insufficient funds for transfer.\n";
        return;
    }

    *balance -= amount;
    *target.balance += amount;
}

double BankAccount::getBalance() const
{
    return *balance;
}

Transaction* BankAccount::getTransaction(int index)
{
    if(index < 0 || index >= static_cast<int>(transactions.size()))
        return nullptr;

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

std::string BankAccount::generateReport() const
{
    char report[128];

    snprintf(report, sizeof(report),
             "Account=%d Owner=%s Balance=%.2f",
             accountId,
             owner.c_str(),
             *balance);

    return std::string(report);
}
