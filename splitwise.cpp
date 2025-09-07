#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <limits>
#include <cmath>

using namespace std;

enum SplitType {
    EQUAL,
    PERCENTAGE,
    CUSTOM_WEIGHT
};

struct Transaction {
    int id;
    string payer;
    double amount;
    vector<string> participants;
    vector<double> weights; // For custom splits
    SplitType splitType;
    string description;
    string date;
    string groupName; // Empty for personal transactions
    bool isSettled;
    
    Transaction(int _id, string _payer, double _amount, vector<string> _participants, 
                string _description, string _groupName = "", SplitType _splitType = EQUAL, 
                vector<double> _weights = {}) 
        : id(_id), payer(_payer), amount(_amount), participants(_participants), 
          description(_description), groupName(_groupName), splitType(_splitType), 
          weights(_weights), isSettled(false) {
        
        // Get current date
        time_t now = time(0);
        char* dt = ctime(&now);
        date = string(dt);
        date.pop_back(); // Remove newline
    }
};

struct Settlement {
    int transactionId;
    string from;
    string to;
    double amount;
    string date;
    string groupName;
    
    Settlement(int _transactionId, string _from, string _to, double _amount, string _groupName = "") 
        : transactionId(_transactionId), from(_from), to(_to), amount(_amount), groupName(_groupName) {
        time_t now = time(0);
        char* dt = ctime(&now);
        date = string(dt);
        date.pop_back(); // Remove newline
    }
};

class SplitWiseApp {
private:
    vector<Transaction> transactions;
    vector<Settlement> settlements;
    vector<string> groups;
    int nextTransactionId;
    
public:
    SplitWiseApp() : nextTransactionId(1) {}
    
    int getSafeInteger(const string& prompt) {
        int value;
        while (true) {
            cout << prompt;
            cin >> value;
            
            if (cin.fail()) {
                cin.clear(); // Clear error flag
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear buffer
                cout << "Invalid input! Please enter a valid number.\n";
            } else {
                cin.ignore(); // Clear remaining newline
                return value;
            }
        }
    }
    
    double getSafeDouble(const string& prompt) {
        double value;
        while (true) {
            cout << prompt;
            cin >> value;
            
            if (cin.fail()) {
                cin.clear(); // Clear error flag
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear buffer
                cout << "Invalid input! Please enter a valid amount.\n";
            } else {
                cin.ignore(); // Clear remaining newline
                return value;
            }
        }
    }
    
    void addTransaction() {
        cout << "\n--- Add Transaction ---\n";
        
        string payer, description, groupChoice;
        double amount;
        vector<string> participants;
        
        cout << "Enter payer name: ";
        cin.ignore();
        getline(cin, payer);
        
        amount = getSafeDouble("Enter amount: Rs.");
        
        cout << "Enter description: ";
        cin.ignore();
        getline(cin, description);
        
        cout << "Is this a group transaction? (y/n): ";
        char isGroup;
        cin >> isGroup;
        
        string groupName = "";
        if (tolower(isGroup) == 'y') {
            cout << "Available groups: ";
            for (const auto& group : groups) {
                cout << group << " ";
            }
            cout << "\nEnter group name (or create new): ";
            cin.ignore();
            getline(cin, groupName);
            
            // Add group if doesn't exist
            if (find(groups.begin(), groups.end(), groupName) == groups.end()) {
                groups.push_back(groupName);
                cout << "Created new group: " << groupName << "\n";
            }
        }
        

        cout << "Enter participants (comma separated): ";
        cin.ignore();
        string participantInput;
        getline(cin, participantInput);
        
        // Parse participants
        stringstream ss(participantInput);
        string participant;
        while (getline(ss, participant, ',')) {
            // Trim whitespace
            participant.erase(0, participant.find_first_not_of(" \t"));
            participant.erase(participant.find_last_not_of(" \t") + 1);
            if (!participant.empty()) {
                participants.push_back(participant);
            }
        }
        
        // Add payer to participants if not already included
        if (find(participants.begin(), participants.end(), payer) == participants.end()) {
            participants.push_back(payer);
        }
        
        // Choose split type
        SplitType splitType = EQUAL;
        vector<double> weights;
        
        cout << "Choose split type:\n";
        cout << "1. Equal split\n";
        cout << "2. Percentage split\n";
        cout << "3. Custom weight split\n";
        
        int choice = getSafeInteger("Enter choice (1-3): ");
        
        switch (choice) {
            case 1:
                splitType = EQUAL;
                break;
            case 2:
                splitType = PERCENTAGE;
                cout << "Enter percentages for each participant:\n";
                for (const auto& p : participants) {
                    double percentage = getSafeDouble(p + ": ");
                    weights.push_back(percentage);
                }
                break;
            case 3:
                splitType = CUSTOM_WEIGHT;
                cout << "Enter weights for each participant:\n";
                for (const auto& p : participants) {
                    double weight = getSafeDouble(p + ": ");
                    weights.push_back(weight);
                }
                break;
            default:
                splitType = EQUAL;
        }
        
        Transaction newTransaction(nextTransactionId++, payer, amount, participants, 
                                 description, groupName, splitType, weights);
        transactions.push_back(newTransaction);
        
        cout << "Transaction added successfully! ID: " << newTransaction.id << "\n";
    }
    
    void deleteTransaction() {
        cout << "\n--- Delete Transaction ---\n";
        showAllTransactions();
        
        int id = getSafeInteger("Enter transaction ID to delete: ");
        
        auto it = find_if(transactions.begin(), transactions.end(),
                         [id](const Transaction& t) { return t.id == id; });
        
        if (it != transactions.end()) {
            transactions.erase(it);
            cout << "Transaction deleted successfully!\n";
        } else {
            cout << "Transaction not found!\n";
        }
    }
    
    map<string, double> calculateNetBalance(const string& groupName = "") {
        map<string, double> netBalance;
        
        for (const auto& transaction : transactions) {
            if (transaction.isSettled) continue;
            
            // Filter by group if specified
            if (!groupName.empty() && transaction.groupName != groupName) continue;
            
            double perPersonAmount = 0;
            
            // Calculate how much each person owes
            switch (transaction.splitType) {
                case EQUAL:
                    perPersonAmount = transaction.amount / transaction.participants.size();
                    for (const auto& participant : transaction.participants) {
                        netBalance[participant] -= perPersonAmount;
                    }
                    break;
                    
                case PERCENTAGE:
                    for (size_t i = 0; i < transaction.participants.size(); i++) {
                        double owedAmount = transaction.amount * (transaction.weights[i] / 100.0);
                        netBalance[transaction.participants[i]] -= owedAmount;
                    }
                    break;
                    
                case CUSTOM_WEIGHT:
                    {
                        double totalWeight = 0;
                        for (double weight : transaction.weights) {
                            totalWeight += weight;
                        }
                        for (size_t i = 0; i < transaction.participants.size(); i++) {
                            double owedAmount = transaction.amount * (transaction.weights[i] / totalWeight);
                            netBalance[transaction.participants[i]] -= owedAmount;
                        }
                    }
                    break;
            }
            
            // Add the amount paid by the payer
            netBalance[transaction.payer] += transaction.amount;
        }
        
        // Apply settlements - subtract settled amounts from balances
        for (const auto& settlement : settlements) {
            // Filter by group if specified
            if (!groupName.empty() && settlement.groupName != groupName) continue;
            
            // Settlement reduces the debt of the payer and the credit of the receiver
            netBalance[settlement.from] += settlement.amount;  // Debtor paid, so their debt reduces
            netBalance[settlement.to] -= settlement.amount;    // Creditor received, so their credit reduces
        }
        
        return netBalance;
    }
    
    void showBalances() {
        cout << "\n--- Show Balances ---\n";
        cout << "1. All balances\n";
        cout << "2. Group balances\n";
        
        int choice = getSafeInteger("Enter choice: ");
        
        string groupName = "";
        if (choice == 2) {
            cout << "Available groups: ";
            for (const auto& group : groups) {
                cout << group << " ";
            }
            cout << "\nEnter group name: ";
            cin.ignore();
            getline(cin, groupName);
        }
        
        map<string, double> balances = calculateNetBalance(groupName);
        
        cout << "\n=== Net Balances ===\n";
        cout << setprecision(2) << fixed;
        
        for (const auto& balance : balances) {
            cout << balance.first << ": ";
            if (balance.second > 0.01) {
                cout << "Gets Rs." << balance.second << "\n";
            } else if (balance.second < -0.01) {
                cout << "Owes Rs." << -balance.second << "\n";
            } else {
                cout << "Settled\n";
            }
        }
    }
    
    vector<pair<string, string>> minimizeTransactions(const string& groupName = "") {
        map<string, double> netBalance = calculateNetBalance(groupName);
        vector<pair<string, string>> settlements;
        
        vector<pair<string, double>> creditors, debtors;
        
        for (const auto& balance : netBalance) {
            if (balance.second > 0.01) {
                creditors.push_back({balance.first, balance.second});
            } else if (balance.second < -0.01) {
                debtors.push_back({balance.first, -balance.second});
            }
        }
        
        cout << "\n=== Optimized Settlement Plan ===\n";
        cout << setprecision(2) << fixed;
        
        if (creditors.empty() && debtors.empty()) {
            cout << "All settlements are complete! No pending transactions.\n";
            return settlements;
        }
        
        size_t i = 0, j = 0;
        while (i < creditors.size() && j < debtors.size()) {
            double settleAmount = min(creditors[i].second, debtors[j].second);
            
            cout << debtors[j].first << " ---> " << creditors[i].first 
                 << ": Rs." << settleAmount << "\n";
            
            settlements.push_back({debtors[j].first, creditors[i].first});
            
            creditors[i].second -= settleAmount;
            debtors[j].second -= settleAmount;
            
            if (creditors[i].second < 0.01) i++;
            if (debtors[j].second < 0.01) j++;
        }
        
        return settlements;
    }
    
    void settleDebt() {
        cout << "\n--- Settle Debt ---\n";
        
        // First show current balances to help user understand what needs to be settled
        cout << "Current outstanding balances:\n";
        map<string, double> currentBalances = calculateNetBalance();
        cout << setprecision(2) << fixed;
        
        for (const auto& balance : currentBalances) {
            if (fabs(balance.second) > 0.01) {
                cout << balance.first << ": ";
                if (balance.second > 0) {
                    cout << "Gets Rs." << balance.second << "\n";
                } else {
                    cout << "Owes Rs." << -balance.second << "\n";
                }
            }
        }
        
        string from, to, groupName;
        double amount;
        
        cout << "\nEnter debtor name (who is paying): ";
        cin.ignore();
        getline(cin, from);
        
        cout << "Enter creditor name (who is receiving): ";
        getline(cin, to);
        
        amount = getSafeDouble("Enter settlement amount: Rs.");
        
        cout << "Is this for a group? (y/n): ";
        char isGroup;
        cin >> isGroup;
        
        if (tolower(isGroup) == 'y') {
            cout << "Available groups: ";
            for (const auto& group : groups) {
                cout << group << " ";
            }
            cout << "\nEnter group name: ";
            cin.ignore();
            getline(cin, groupName);
        }
        
        // Validate settlement
        map<string, double> balances = calculateNetBalance(groupName);
        
        // Check if the debtor actually owes money
        if (balances[from] >= -0.01) {
            cout << "Warning: " << from << " doesn't owe money";
            if (!groupName.empty()) cout << " in group " << groupName;
            cout << ".\n";
        }
        
        // Check if the creditor is owed money
        if (balances[to] <= 0.01) {
            cout << "Warning: " << to << " is not owed money";
            if (!groupName.empty()) cout << " in group " << groupName;
            cout << ".\n";
        }
        
        // Check if settlement amount is reasonable
        double maxSettleable = min(fabs(min(0.0, balances[from])), max(0.0, balances[to]));
        if (amount > maxSettleable + 0.01) {
            cout << "Warning: Settlement amount (Rs." << amount 
                 << ") is more than the outstanding debt (Rs." << maxSettleable << ").\n";
            cout << "Do you want to continue? (y/n): ";
            char confirm;
            cin >> confirm;
            if (tolower(confirm) != 'y') {
                cout << "Settlement cancelled.\n";
                return;
            }
        }
        
        // Record settlement
        Settlement settlement(0, from, to, amount, groupName);
        settlements.push_back(settlement);
        
        cout << "Settlement recorded successfully!\n";
        cout << from << " paid Rs." << amount << " to " << to;
        if (!groupName.empty()) {
            cout << " for group: " << groupName;
        }
        cout << "\n";
        
        // Show updated balances after settlement
        cout << "\nUpdated balances after settlement:\n";
        map<string, double> updatedBalances = calculateNetBalance(groupName);
        
        bool hasOutstanding = false;     
        for (const auto& balance : updatedBalances) {
            if (fabs(balance.second) > 0.01) {
                cout << balance.first << ": ";
                if (balance.second > 0) {
                    cout << "Gets Rs." << balance.second << "\n";
                } else {
                    cout << "Owes Rs." << -balance.second << "\n";
                }
                hasOutstanding = true;
            }
        }
        
        if (!hasOutstanding) {
            cout << "🎉 All debts settled!";
            if (!groupName.empty()) {
                cout << " for group " << groupName;
            }
            cout << "\n";
        }
    }
    
    void showAllTransactions() {
        cout << "\n=== All Transactions ===\n";
        cout << setprecision(2) << fixed;
        
        if (transactions.empty()) {
            cout << "No transactions found.\n";
            return;
        }
        
        for (const auto& transaction : transactions) {
            cout << "ID: " << transaction.id << " | " << transaction.payer 
                 << " paid Rs." << transaction.amount;
            
            if (!transaction.groupName.empty()) {
                cout << " [Group: " << transaction.groupName << "]";
            }
            
            cout << "\n  Description: " << transaction.description;
            cout << "\n  Participants: ";
            for (size_t i = 0; i < transaction.participants.size(); i++) {
                cout << transaction.participants[i];
                if (i < transaction.participants.size() - 1) cout << ", ";
            }
            cout << "\n  Date: " << transaction.date;
            cout << "\n  Status: " << (transaction.isSettled ? "Settled" : "Active") << "\n";
            cout << "----------------------------------------\n";
        }
    }
    
    void searchTransactions() {
        cout << "\n--- Search/Filter Transactions ---\n";
        cout << "1. Filter by person\n";
        cout << "2. Filter by group\n";
        cout << "3. Filter by amount range\n";
        
        int choice = getSafeInteger("Enter choice: ");
        
        vector<Transaction> filtered;
        
        switch (choice) {
            case 1: {
                string person;
                cout << "Enter person name: ";
                cin.ignore();
                getline(cin, person);
                
                for (const auto& t : transactions) {
                    if (t.payer == person || 
                        find(t.participants.begin(), t.participants.end(), person) != t.participants.end()) {
                        filtered.push_back(t);
                    }
                }
                break;
            }
            case 2: {
                string group;
                cout << "Enter group name: ";
                cin.ignore();
                getline(cin, group);
                
                for (const auto& t : transactions) {
                    if (t.groupName == group) {
                        filtered.push_back(t);
                    }
                }
                break;
            }
            case 3: {
                double minAmount = getSafeDouble("Enter minimum amount: Rs.");
                double maxAmount = getSafeDouble("Enter maximum amount: Rs.");
                
                for (const auto& t : transactions) {
                    if (t.amount >= minAmount && t.amount <= maxAmount) {
                        filtered.push_back(t);
                    }
                }
                break;
            }
        }
        
        cout << "\n=== Filtered Results ===\n";
        if (filtered.empty()) {
            cout << "No transactions found matching the criteria.\n";
            return;
        }
        
        for (const auto& transaction : filtered) {
            cout << "ID: " << transaction.id << " | " << transaction.payer 
                 << " paid Rs." << transaction.amount;
            
            if (!transaction.groupName.empty()) {
                cout << " [Group: " << transaction.groupName << "]";
            }
            cout << "\n  Description: " << transaction.description << "\n";
            cout << "----------------------------------------\n";
        }
    }
    
    void showPersonalTransactions() {
        cout << "\n--- Personal View ---\n";
        string person;
        cout << "Enter your name: ";
        cin.ignore();
        getline(cin, person);
        
        cout << "\n=== Your Transactions ===\n";
        cout << setprecision(2) << fixed;
        
        bool hasTransactions = false;
        for (const auto& transaction : transactions) {
            bool involved = (transaction.payer == person || 
                           find(transaction.participants.begin(), transaction.participants.end(), person) 
                           != transaction.participants.end());
            
            if (involved) {
                hasTransactions = true;
                cout << "ID: " << transaction.id << " | ";

                // Calculate this person's share in the transaction
                double personShare = 0.0;
                switch (transaction.splitType) {
                    case EQUAL:
                        personShare = transaction.amount / transaction.participants.size();
                        break;
                    case PERCENTAGE:
                        for (size_t i = 0; i < transaction.participants.size(); i++) {
                            if (transaction.participants[i] == person) {
                                personShare = transaction.amount * (transaction.weights[i] / 100.0);
                                break;
                            }
                        }
                        break;
                    case CUSTOM_WEIGHT:
                        {
                            double totalWeight = 0;
                            for (double weight : transaction.weights) {
                                totalWeight += weight;
                            }
                            for (size_t i = 0; i < transaction.participants.size(); i++) {
                                if (transaction.participants[i] == person) {
                                    personShare = transaction.amount * (transaction.weights[i] / totalWeight);
                                    break;
                                }
                            }
                        }
                        break;
                }

                if (transaction.payer == person) {
                    cout << "You paid Rs." << transaction.amount 
                         << " (Your share: Rs." << personShare << ")";
                } else {
                    cout << transaction.payer << " paid Rs." << transaction.amount 
                         << " (Your share: Rs." << personShare << ")";
                }

                if (!transaction.groupName.empty()) {
                    cout << " [Group: " << transaction.groupName << "]";
                } else {
                    cout << " [Personal]";
                }

                cout << "\n  Description: " << transaction.description << "\n";
                cout << "----------------------------------------\n";
            }
        }
        
        if (!hasTransactions) {
            cout << "No transactions found for " << person << ".\n";
        }
        
        // Show personal balance
        map<string, double> allBalances = calculateNetBalance();
        if (allBalances.find(person) != allBalances.end()) {
            cout << "\nYour overall balance: ";
            double balance = allBalances[person];
            if (balance > 0.01) {
                cout << "You get Rs." << balance << "\n";
            } else if (balance < -0.01) {
                cout << "You owe Rs." << -balance << "\n";
            } else {
                cout << "Settled\n";
            }
        } else {
            cout << "\nYour overall balance: Settled\n";
        }
    }
    
    void showSettlementHistory() {
        cout << "\n=== Settlement History ===\n";
        cout << setprecision(2) << fixed;
        
        if (settlements.empty()) {
            cout << "No settlements recorded yet.\n";
            return;
        }
        
        for (const auto& settlement : settlements) {
            cout << settlement.from << " ---> " << settlement.to 
                 << ": Rs." << settlement.amount;
            
            if (!settlement.groupName.empty()) {
                cout << " [Group: " << settlement.groupName << "]";
            } else {
                cout << " [Personal]";
            }
            
            cout << "\n  Date: " << settlement.date << "\n";
            cout << "----------------------------------------\n";
        }
    }
    
    void showMenu() {
        cout << "\n======= SplitWise Clone =======\n";
        cout << "1.  Add Transaction\n";
        cout << "2.  Delete Transaction\n";
        cout << "3.  Show Balances\n";
        cout << "4.  Minimize Transactions\n";
        cout << "5.  Settle Debt\n";
        cout << "6.  Show All Transactions\n";
        cout << "7.  Search/Filter Transactions\n";
        cout << "8.  Personal Transaction View\n";
        cout << "9.  Settlement History\n";
        cout << "10. Exit\n";
        cout << "===============================\n";
    }
    
    void run() {
        cout << "Welcome to SplitWise Clone!\n";
        
        while (true) {
            showMenu();
            
            int choice = getSafeInteger("Enter your choice: ");
            
            switch (choice) {
                case 1:
                    addTransaction();
                    break;
                case 2:
                    deleteTransaction();
                    break;
                case 3:
                    showBalances();
                    break;
                case 4:
                    {
                        cout << "1. Minimize all transactions\n";
                        cout << "2. Minimize group transactions\n";
                        int minChoice = getSafeInteger("Enter choice: ");
                        
                        if (minChoice == 2) {
                            cout << "Available groups: ";
                            for (const auto& group : groups) {
                                cout << group << " ";
                            }
                            cout << "\nEnter group name: ";
                            string groupName;
                            cin.ignore();
                            getline(cin, groupName);
                            minimizeTransactions(groupName);
                        } else {
                            minimizeTransactions();
                        }
                    }
                    break;
                case 5:
                    settleDebt();
                    break;
                case 6:
                    showAllTransactions();
                    break;
                case 7:
                    searchTransactions();
                    break;
                case 8:
                    showPersonalTransactions();
                    break;
                case 9:
                    showSettlementHistory();
                    break;
                case 10:
                    cout << "Thank you for using SplitWise Clone!\n";
                    return;
                default:
                    cout << "Invalid choice! Please try again.\n";
            }
            
            cout << "\nPress Enter to continue...";
            cin.ignore();
            cin.get();
        }
    }
};

int main() {
    SplitWiseApp app;
    app.run();
    return 0;
}
