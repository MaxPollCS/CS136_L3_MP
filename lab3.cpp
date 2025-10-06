#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <ctime>

using namespace std;

// ----- file names (no prompting) -----
const string INPUT_FILE  = "checking_requests.txt";
const string OUTPUT_FILE = "created_accounts.txt";
const string ERROR_FILE  = "invalid_records.txt";
const string LOG_FILE    = "processing.log";

// ----- knobs you might tweak later -----
const int    MAX_STACK         = 1000;
const int    MAX_INVALID       = 1000;
const int    SSN_LEN           = 10;
const int    NAME_MIN_LEN      = 2;
const int    USER_MIN_LEN      = 4;
const int    MAIL_MIN_LEN      = 4;
const int    SEQ_MAX           = 99;
const double BONUS_EDU         = 150.00;
const double BONUS_NON_EDU     = 100.00;
const double AVAILABLE_BAL     = 0.00;

// ----- table widths -----
const int W_SSN    = 12;
const int W_FNAME  = 14;
const int W_LNAME  = 14;
const int W_EMAIL  = 30;
const int W_ACCT   = 12;
const int W_AVAIL  = 12;
const int W_PRESENT= 14;

// ----- sequential last-two-digits counter for account numbers -----
int gSeqCounter = 0;

// simple invalid bucket for printing later
struct InvalidRecord {
    string line;
};

void logMessage(const string& msg) {
    ofstream log(LOG_FILE.c_str(), ios::app);
    if (!log) return;
    time_t t = time(0);
    tm* lt = localtime(&t);
    ostringstream ts;
    ts << setfill('0')
       << (1900 + lt->tm_year) << "-"
       << setw(2) << (1 + lt->tm_mon) << "-"
       << setw(2) << lt->tm_mday << " "
       << setw(2) << lt->tm_hour << ":"
       << setw(2) << lt->tm_min << ":"
       << setw(2) << lt->tm_sec;
    log << "[" << ts.str() << "] " << msg << "\n";
}

bool isAllDigits(const string& s) {
    if (s.length() == 0) return false;
    for (size_t i = 0; i < s.length(); ++i) {
        if (!isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}

bool isAlphaMin(const string& s, int minLen) {
    if ((int)s.length() < minLen) return false;
    for (size_t i = 0; i < s.length(); ++i) {
        if (!isalpha(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}

bool isUserValid(const string& user) {
    if ((int)user.length() < USER_MIN_LEN) return false;
    for (size_t i = 0; i < user.length(); ++i) {
        char c = user[i];
        if (!(isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_')) return false;
    }
    return true;
}

bool isMailValid(const string& mail) {
    if ((int)mail.length() < MAIL_MIN_LEN) return false;
    for (size_t i = 0; i < mail.length(); ++i) {
        if (!isalpha(static_cast<unsigned char>(mail[i]))) return false;
    }
    return true;
}

string toLowerCopy(const string& s) {
    string r = s;
    for (size_t i = 0; i < r.length(); ++i) r[i] = static_cast<char>(tolower(static_cast<unsigned char>(r[i])));
    return r;
}

bool splitEmail(const string& email, string& user, string& mail, string& domain) {
    size_t atPos = email.find('@');
    if (atPos == string::npos) return false;
    size_t dotPos = email.find('.', atPos + 1);
    if (dotPos == string::npos) return false;

    user   = email.substr(0, atPos);
    mail   = email.substr(atPos + 1, dotPos - (atPos + 1));
    domain = email.substr(dotPos + 1);

    return (user.length() > 0 && mail.length() > 0 && domain.length() > 0);
}

bool isEmailValid(const string& email) {
    string u, m, d;
    if (!splitEmail(email, u, m, d)) return false;
    string dl = toLowerCopy(d);
    if (!isUserValid(u)) return false;
    if (!isMailValid(m)) return false;
    if (!(dl == "com" || dl == "edu")) return false;
    return true;
}

bool isAccountNumValid(const string& acct) {
    if (acct.length() != 8) return false;
    return isAllDigits(acct);
}

string random6Digits() {
    // allow leading zeros
    ostringstream oss;
    for (int i = 0; i < 6; ++i) {
        int d = rand() % 10;
        oss << d;
    }
    return oss.str();
}

string nextTwoDigitSeq(bool& ok) {
    if (gSeqCounter >= SEQ_MAX) { ok = false; return ""; }
    ++gSeqCounter; // 1..99
    ostringstream oss;
    oss << setfill('0') << setw(2) << gSeqCounter;
    ok = true;
    return oss.str();
}

double startingPresentByEmail(const string& email) {
    string u, m, d;
    if (!splitEmail(email, u, m, d)) return BONUS_NON_EDU;
    string dl = toLowerCopy(d);
    if (dl == "edu") return BONUS_EDU;
    return BONUS_NON_EDU;
}

class CheckingAccount {
    string ssn;
    string first;
    string last;
    string email;
    string accountNumber;
    double available;
    double present;

public:
    CheckingAccount() {
        setAll("0000000000", "NA", "NA", "userx@abcd.com", "00000000", 0.00, 0.00);
    }

    CheckingAccount(const string& s, const string& f, const string& l,
                    const string& e, const string& acct,
                    double avail, double pres) {
        setAll(s, f, l, e, acct, avail, pres);
    }

    bool setAll(const string& s, const string& f, const string& l,
                const string& e, const string& acct,
                double avail, double pres) {
        if ((int)s.length() != SSN_LEN) return false;
        if (!isAllDigits(s)) return false;
        if (!isAlphaMin(f, NAME_MIN_LEN)) return false;
        if (!isAlphaMin(l, NAME_MIN_LEN)) return false;
        if (!isEmailValid(e)) return false;
        if (!isAccountNumValid(acct)) return false;
        if (avail < 0.0) return false;
        if (pres  < 0.0) return false;

        ssn = s;
        first = f;
        last = l;
        email = e;
        accountNumber = acct;
        available = avail;
        present = pres;
        return true;
    }

    const string& getSSN() const { return ssn; }
    const string& getFirst() const { return first; }
    const string& getLast() const { return last; }
    const string& getEmail() const { return email; }
    const string& getAccount() const { return accountNumber; }
    double getAvailable() const { return available; }
    double getPresent() const { return present; }
};

class AccountStack {
    CheckingAccount data[MAX_STACK];
    int topIndex; // -1 when empty
public:
    AccountStack(): topIndex(-1) {}

    bool push(const CheckingAccount& acc) {
        if (isFull()) return false;
        data[++topIndex] = acc;
        return true;
    }

    bool pop(CheckingAccount& out) {
        if (isEmpty()) return false;
        out = data[topIndex--];
        return true;
    }

    bool peek(CheckingAccount& out) const {
        if (isEmpty()) return false;
        out = data[topIndex];
        return true;
    }

    bool isEmpty() const { return topIndex < 0; }
    bool isFull()  const { return topIndex >= (MAX_STACK - 1); }
    int  size()    const { return topIndex + 1; }
};

// printing helpers
void printAccountsHeader(ostream& os) {
    os << left
       << setw(W_SSN)    << "SSN"
       << setw(W_FNAME)  << "First"
       << setw(W_LNAME)  << "Last"
       << setw(W_EMAIL)  << "Email"
       << setw(W_ACCT)   << "Acct#"
       << right
       << setw(W_AVAIL)  << "Avail"
       << setw(W_PRESENT)<< "Present"
       << "\n";

    int total = W_SSN + W_FNAME + W_LNAME + W_EMAIL + W_ACCT + W_AVAIL + W_PRESENT;
    for (int i = 0; i < total; ++i) os << '-';
    os << "\n";
}

void printAccountRow(ostream& os, const CheckingAccount& a) {
    os << left
       << setw(W_SSN)    << a.getSSN()
       << setw(W_FNAME)  << a.getFirst()
       << setw(W_LNAME)  << a.getLast()
       << setw(W_EMAIL)  << a.getEmail()
       << setw(W_ACCT)   << a.getAccount()
       << right << fixed << setprecision(2)
       << setw(W_AVAIL)  << a.getAvailable()
       << setw(W_PRESENT)<< a.getPresent()
       << "\n";
}

void printAllAccountsNonDestructive(AccountStack& stack, ostream& os) {
    AccountStack temp;
    CheckingAccount cur;
    printAccountsHeader(os);

    // move to temp to access all
    while (stack.pop(cur)) {
        temp.push(cur);
    }
    // print while restoring
    while (temp.pop(cur)) {
        printAccountRow(os, cur);
        stack.push(cur);
    }
}

void writeAllAccountsAndEmpty(AccountStack& stack, ostream& os) {
    CheckingAccount cur;
    printAccountsHeader(os);
    while (stack.pop(cur)) {
        printAccountRow(os, cur);
    }
}

bool buildAccount(const string& ssn, const string& first,
                  const string& last, const string& email,
                  CheckingAccount& out, string& reason) {
    if ((int)ssn.length() != SSN_LEN || !isAllDigits(ssn)) { reason = "bad SSN"; return false; }
    if (!isAlphaMin(first, NAME_MIN_LEN)) { reason = "bad first name"; return false; }
    if (!isAlphaMin(last, NAME_MIN_LEN))  { reason = "bad last name"; return false; }
    if (!isEmailValid(email))             { reason = "bad email"; return false; }

    bool okSeq = true;
    string seq = nextTwoDigitSeq(okSeq);
    if (!okSeq) { reason = "account sequence exhausted"; return false; }

    string acct = random6Digits() + seq;
    double avail = AVAILABLE_BAL;
    double pres  = startingPresentByEmail(email);

    CheckingAccount tmp;
    if (!tmp.setAll(ssn, first, last, email, acct, avail, pres)) {
        reason = "failed to set account";
        return false;
    }
    out = tmp;
    return true;
}

void printInvalidRecords(const InvalidRecord inv[], int count, ostream& os) {
    if (count == 0) {
        os << "(none)\n";
        return;
    }
    os << left << setw(W_SSN + W_FNAME + W_LNAME + W_EMAIL + 10) << "Invalid Record" << "\n";
    int total = W_SSN + W_FNAME + W_LNAME + W_EMAIL + 10;
    for (int i = 0; i < total; ++i) os << '-';
    os << "\n";
    for (int i = 0; i < count; ++i) {
        os << inv[i].line << "\n";
    }
}

void printLogFile() {
    ifstream in(LOG_FILE.c_str());
    if (!in) {
        cout << "(no log yet)\n";
        return;
    }
    string line;
    while (std::getline(in, line)) {
        cout << line << "\n";
    }
}

int main() {
    srand(static_cast<unsigned int>(time(0)));

    AccountStack accounts;
    InvalidRecord invalids[MAX_INVALID];
    int invalidCount = 0;

    bool processed = false;

    int choice = -1;
    do {
        cout << "\n=== Checking Account Night Processor ===\n";
        cout << "1) Process all new requests\n";
        cout << "2) Print created accounts\n";
        cout << "3) Print invalid records\n";
        cout << "4) Print log\n";
        cout << "5) Quit and write created accounts to file\n";
        cout << "Select: ";
        if (!(cin >> choice)) {
            cin.clear();
            string dump;
            std::getline(cin, dump);
            cout << "Invalid input.\n";
            continue;
        }

        if (choice == 1) {
            if (processed) {
                cout << "Already processed for this run.\n";
                continue;
            }
            ofstream errOut(ERROR_FILE.c_str()); // overwrite each run
            if (!errOut) {
                cout << "Could not open error file.\n";
                continue;
            }
            ifstream in(INPUT_FILE.c_str());
            if (!in) {
                cout << "Could not open input file: " << INPUT_FILE << "\n";
                logMessage("FAILED to open input file");
                continue;
            }

            int processedCount = 0;
            int createdCount   = 0;
            int badCount       = 0;

            string ssn, first, last, email;
            while (in >> ssn >> first >> last >> email) {
                ++processedCount;

                if (accounts.isFull()) {
                    string msg = "stack full; remaining records skipped";
                    logMessage(msg);
                    cout << "Stack is full. Stopping.\n";
                    break;
                }

                CheckingAccount acc;
                string reason;
                bool ok = buildAccount(ssn, first, last, email, acc, reason);
                if (ok) {
                    if (!accounts.push(acc)) {
                        // shouldn't happen with the isFull check
                        errOut << ssn << " " << first << " " << last << " " << email
                               << "  | reason: stack push failed\n";
                        if (invalidCount < MAX_INVALID) {
                            invalids[invalidCount++].line = ssn + " " + first + " " + last + " " + email + "  | reason: stack push failed";
                        }
                        ++badCount;
                        logMessage("push failed for " + email);
                    } else {
                        ++createdCount;
                        logMessage("created account " + acc.getAccount() + " for " + email);
                    }
                } else {
                    errOut << ssn << " " << first << " " << last << " " << email
                           << "  | reason: " << reason << "\n";
                    if (invalidCount < MAX_INVALID) {
                        invalids[invalidCount++].line = ssn + " " + first + " " + last + " " + email + "  | reason: " + reason;
                    }
                    ++badCount;
                    logMessage("invalid record for " + email + " (" + reason + ")");
                }
            }

            cout << "\nProcessed: " << processedCount
                 << " | Created: "  << createdCount
                 << " | Invalid: "  << badCount << "\n";

            ostringstream s;
            s << "SUMMARY processed=" << processedCount << " created=" << createdCount << " invalid=" << badCount;
            logMessage(s.str());
            processed = true;
        }
        else if (choice == 2) {
            if (accounts.isEmpty()) {
                cout << "(no created accounts yet)\n";
            } else {
                printAllAccountsNonDestructive(accounts, cout);
            }
        }
        else if (choice == 3) {
            printInvalidRecords(invalids, invalidCount, cout);
        }
        else if (choice == 4) {
            printLogFile();
        }
        else if (choice == 5) {
            ofstream out(OUTPUT_FILE.c_str());
            if (!out) {
                cout << "Could not open output file: " << OUTPUT_FILE << "\n";
                logMessage("FAILED to open output file");
            } else {
                writeAllAccountsAndEmpty(accounts, out);
                logMessage("wrote all created accounts to " + OUTPUT_FILE + " and emptied stack");
                cout << "Wrote accounts to " << OUTPUT_FILE << ".\n";
            }
            cout << "Bye.\n";
        }
        else {
            cout << "Choose 1-5.\n";
        }

    } while (choice != 5);

    /*
    --- test runs (example) ---
    Input file (checking_requests.txt):
      1234598768 Mary Lee lee35@lapc.edu
      0123456789 John Doe john_doe44@gmail.com
      9999999999 Al X ax@abed.edu
      1111111111 Ana Smith ana.smith_42@fouru.com
      2222222222 Bo Li bo_li.__@fouru.edu

    Typical console:
      === Checking Account Night Processor ===
      1) Process all new requests
      2) Print created accounts
      3) Print invalid records
      4) Print log
      5) Quit and write created accounts to file
      Select: 1

      Processed: 5 | Created: 3 | Invalid: 2

      Select: 2
      (table of 3 accounts...)

      Select: 3
      (two invalid lines with reasons)

      Select: 4
      (log lines)

      Select: 5
      Wrote accounts to created_accounts.txt.
      Bye.

    Notes:
      - edu emails get $150 present, others $100.
      - SSN must be exactly 10 digits.
      - Names must be letters only, at least 2 chars.
      - Username in email must be 4+ and only [A-Za-z0-9._]
      - Mailserver must be 4+ letters, domain is com or edu.
    */
}
