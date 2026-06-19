#include "BankAccount.h"
#include <iostream>
#include <thread>
#include <mutex>

BankAccount* globalAccount = nullptr;
std::mutex accountMutex;

void performTransactions()
{
    for(int i=0;i<100000;i++)
    {
        std::lock_guard<std::mutex> lock(accountMutex);
        globalAccount->deposit(1);
    }
}

int main()
{
    BankAccount account1(1,
                         "Alice",
                         1000);

    BankAccount account2(2,
                         "Bob",
                         500);

    globalAccount = &account1;

    std::thread t1(performTransactions);
    std::thread t2(performTransactions);

    account1.transfer(account2, 5000);

    account1.deposit(-200);

    account1.withdraw(-500);

    Transaction* tx =
        new Transaction(1,
                        200,
                        "Salary");

    account1.addTransaction(tx);

    Transaction* tx2 =
        account1.getTransaction(0);

    if(tx2 != nullptr)
    {
        std::cout
            << tx2->description
            << std::endl;
    }
    else
    {
        std::cout << "Transaction not found.\n";
    }

    std::string report =
        account1.generateReport();

    std::cout
        << report
        << std::endl;

    t1.join();
    t2.join();

    std::cout
        << account1.getBalance()
        << std::endl;

    return 0;
}
