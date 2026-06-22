#include "httplib.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <list> 
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <tuple>

using json = nlohmann::json;

// --- Time Utilities ---
int convertToTimeT(int date) {
    int day = date / 1000000; int month = (date / 10000) % 100; int year = date % 10000;
    struct tm tm = {0}; tm.tm_mday = day; tm.tm_mon = month - 1; tm.tm_year = year - 1900;
    return mktime(&tm);
}
int daysDifference(int date1, int date2) {
    time_t t1 = convertToTimeT(date1); time_t t2 = convertToTimeT(date2);
    return std::abs(difftime(t1, t2) / (60 * 60 * 24));
}

// --- Core OOP Classes ---
class User {
public:
    int id; std::string name; std::string passwd; std::string role;
    int fine; int maxBooks; int maxDays; int fineRate;
    std::vector<std::pair<int, int>> borrowedBooks; // Active: <bookId, dateBorrowed>
    std::vector<std::tuple<int, int, int>> borrowed_History; // Past: <bookId, dateBorrowed, dateReturned>

    User(int id, std::string name, std::string passwd, std::string role, int fine, int maxBooks, int maxDays, int fineRate) 
        : id(id), name(name), passwd(passwd), role(role), fine(fine), maxBooks(maxBooks), maxDays(maxDays), fineRate(fineRate) {}
};

class Book {
public:
    int id; std::string title; std::string author; std::string publisher;
    int year; std::string isbn; std::string status;

    Book(int id, std::string title, std::string author, std::string publisher, int year, std::string isbn, std::string status) 
        : id(id), title(title), author(author), publisher(publisher), year(year), isbn(isbn), status(status) {}
};

// --- Global State ---
std::vector<Book> library_books;
std::list<User> registered_users; 
std::unordered_map<std::string, User*> active_sessions; 

// --- Data Methods ---
void LoadBooks() {
    library_books.clear(); 
    std::ifstream file("books.txt");
    std::string line;
    while (std::getline(file, line)) {
        if(line.empty()) continue;
        std::stringstream ss(line);
        std::string id, title, author, pub, yr, isbn, stat;
        std::getline(ss, id, ','); std::getline(ss, title, ','); std::getline(ss, author, ',');
        std::getline(ss, pub, ','); std::getline(ss, yr, ','); std::getline(ss, isbn, ','); std::getline(ss, stat, ',');
        stat.erase(std::remove(stat.begin(), stat.end(), '\r'), stat.end());
        if(!id.empty()) library_books.emplace_back(std::stoi(id), title, author, pub, std::stoi(yr), isbn, stat);
    }
}
void SaveBooks() {
    std::ofstream file("books.txt");
    for (const auto& b : library_books) file << b.id << "," << b.title << "," << b.author << "," << b.publisher << "," << b.year << "," << b.isbn << "," << b.status << "\n";
}

void LoadUsers() {
    registered_users.clear();
    std::ifstream file("students.txt");
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        int id, fine; std::string name, passwd, historyPart;
        
        // Read base stats
        ss >> id >> name >> passwd >> fine;
        User u(id, name, passwd, "Student", fine, 3, 15, 10);
        
        // Read history (using your original pipe logic)
        std::getline(ss, historyPart);
        if (!historyPart.empty() && historyPart.find('|') != std::string::npos) {
            size_t firstPipe = historyPart.find('|');
            historyPart = historyPart.substr(firstPipe + 1);
            std::stringstream ts(historyPart);
            std::string token; std::vector<std::string> tokens;
            while(std::getline(ts, token, '|')) { if(!token.empty()) tokens.push_back(token); }
            for(size_t i = 0; i + 2 < tokens.size(); i += 3) {
                u.borrowed_History.push_back({std::stoi(tokens[i]), std::stoi(tokens[i+1]), std::stoi(tokens[i+2])});
            }
        }
        registered_users.push_back(u);
    }
    // Admin Account
    registered_users.emplace_back(0, "admin", "admin", "Librarian", 0, 0, 0, 0);
}

void SaveUsers() {
    std::ofstream file("students.txt");
    for (const auto& u : registered_users) {
        if (u.role == "Student") {
            file << u.id << " " << u.name << " " << u.passwd << " " << u.fine << " |";
            for(auto& h : u.borrowed_History) {
                file << std::get<0>(h) << "|" << std::get<1>(h) << "|" << std::get<2>(h) << "|";
            }
            file << "\n";
        }
    }
}

void LoadLoans() {
    std::ifstream file("loans.txt");
    int uid, bid, date; char comma;
    while (file >> uid >> comma >> bid >> comma >> date) {
        for (auto& u : registered_users) { if (u.id == uid) u.borrowedBooks.push_back({bid, date}); }
    }
}
void SaveLoans() {
    std::ofstream file("loans.txt");
    for (const auto& u : registered_users) {
        for (const auto& loan : u.borrowedBooks) file << u.id << "," << loan.first << "," << loan.second << "\n";
    }
}

// --- Helper to find Book Title safely ---
std::string getBookTitle(int id) {
    for (auto& b : library_books) if (b.id == id) return b.title;
    return "Unknown Book";
}

// --- Main Server ---
int main() {
    LoadBooks(); LoadUsers(); LoadLoans();
    httplib::Server svr;

    svr.Post("/api/login", [](const httplib::Request& req, httplib::Response& res) {
        auto x = json::parse(req.body);
        for (auto& user : registered_users) {
            if (user.name == x["username"] && user.passwd == x["password"]) {
                std::string token = "token_" + std::to_string(user.id);
                active_sessions[token] = &user; 
                json res_body = { {"token", token}, {"role", user.role}, {"name", user.name} };
                res.set_content(res_body.dump(), "application/json");
                return;
            }
        }
        res.status = 401;
    });

    // --- NEW: PROFILE & HISTORY FETCH ---
    svr.Post("/api/me", [](const httplib::Request& req, httplib::Response& res) {
        auto x = json::parse(req.body); std::string token = x["token"];
        if (active_sessions.find(token) == active_sessions.end()) { res.status = 401; return; }
        
        User* u = active_sessions[token];
        json resp; resp["fine"] = u->fine;
        
        json borrowed = json::array();
        for(auto& b : u->borrowedBooks) borrowed.push_back({{"id", b.first}, {"title", getBookTitle(b.first)}, {"date", b.second}});
        resp["active_borrows"] = borrowed;
        
        json history = json::array();
        for(auto& h : u->borrowed_History) history.push_back({{"id", std::get<0>(h)}, {"title", getBookTitle(std::get<0>(h))}, {"borrow_date", std::get<1>(h)}, {"return_date", std::get<2>(h)}});
        resp["history"] = history;
        
        res.set_content(resp.dump(), "application/json");
    });

    svr.Get("/api/books", [](const httplib::Request&, httplib::Response& res) {
        json arr = json::array();
        for (const auto& b : library_books) arr.push_back({{"id", b.id}, {"title", b.title}, {"author", b.author}, {"publisher", b.publisher}, {"year", b.year}, {"isbn", b.isbn}, {"status", b.status}});
        res.set_content(arr.dump(), "application/json");
    });

    svr.Post("/api/borrow", [](const httplib::Request& req, httplib::Response& res) {
        auto x = json::parse(req.body); std::string token = x["token"];
        if (active_sessions.find(token) == active_sessions.end()) { res.status = 401; return; }
        User* user = active_sessions[token]; int bookId = x["book_id"]; int currentDate = x["current_date"];
        if (user->fine > 0) { res.status = 400; res.set_content("Cannot borrow: You have unpaid fines.", "text/plain"); return; }
        if (user->borrowedBooks.size() >= user->maxBooks) { res.status = 400; res.set_content("Cannot borrow: Max books limit reached.", "text/plain"); return; }
        for (auto& book : library_books) {
            if (book.id == bookId && book.status == "Available") {
                book.status = "Borrowed"; user->borrowedBooks.push_back({book.id, currentDate});
                SaveBooks(); SaveLoans();
                res.status = 200; res.set_content("Book borrowed successfully!", "text/plain"); return;
            }
        }
    });

    // UPDATED: Push to history on return
    svr.Post("/api/return", [](const httplib::Request& req, httplib::Response& res) {
        auto x = json::parse(req.body); std::string token = x["token"];
        if (active_sessions.find(token) == active_sessions.end()) { res.status = 401; return; }
        User* user = active_sessions[token]; int bookId = x["book_id"]; int returnDate = x["current_date"];
        for (auto it = user->borrowedBooks.begin(); it != user->borrowedBooks.end(); ++it) {
            if (it->first == bookId) {
                int days = daysDifference(returnDate, it->second);
                if (days > user->maxDays) user->fine += (days - user->maxDays) * user->fineRate; 
                
                // Add to persistent history
                user->borrowed_History.push_back({bookId, it->second, returnDate});
                
                user->borrowedBooks.erase(it); SaveUsers(); SaveLoans();
                for (auto& book : library_books) { if (book.id == bookId) book.status = "Available"; }
                SaveBooks();
                json response = { {"message", "Returned! Days held: " + std::to_string(days)} };
                res.set_content(response.dump(), "application/json"); return;
            }
        }
    });

    svr.Post("/api/payfine", [](const httplib::Request& req, httplib::Response& res) {
        std::string token = json::parse(req.body)["token"];
        if (active_sessions.find(token) != active_sessions.end()) { active_sessions[token]->fine = 0; SaveUsers(); res.status = 200; }
    });

    // --- NEW: LIBRARIAN USER MANAGEMENT ---
    svr.Post("/api/get_users", [](const httplib::Request& req, httplib::Response& res) {
        auto x = json::parse(req.body);
        if (active_sessions.find(x["token"]) == active_sessions.end() || active_sessions[x["token"]]->role != "Librarian") { res.status = 403; return; }
        json arr = json::array();
        for (auto& u : registered_users) {
            if (u.role == "Student") arr.push_back({{"id", u.id}, {"name", u.name}, {"fine", u.fine}, {"active", u.borrowedBooks.size()}});
        }
        res.set_content(arr.dump(), "application/json");
    });

    svr.Post("/api/add_user", [](const httplib::Request& req, httplib::Response& res) {
        auto x = json::parse(req.body);
        if (active_sessions.find(x["token"]) == active_sessions.end() || active_sessions[x["token"]]->role != "Librarian") { res.status = 403; return; }
        registered_users.emplace_back(x["id"], x["name"], x["password"], "Student", 0, 3, 15, 10);
        SaveUsers();
        res.status = 200; res.set_content("Student added successfully!", "text/plain");
    });

    // Delete Book and Delete User endpoints remain the same
    svr.Post("/api/delete_book", [](const httplib::Request& req, httplib::Response& res) {
        auto x = json::parse(req.body);
        int bookId = x["book_id"];
        auto it = std::remove_if(library_books.begin(), library_books.end(), [bookId](Book& b){ return b.id == bookId; });
        if (it != library_books.end()) { library_books.erase(it, library_books.end()); SaveBooks(); res.status = 200; }
    });
    svr.Post("/api/delete_user", [](const httplib::Request& req, httplib::Response& res) {
        auto x = json::parse(req.body); int targetId = x["target_id"];
        auto it = std::find_if(registered_users.begin(), registered_users.end(), [targetId](User& u){ return u.id == targetId; });
        if (it != registered_users.end()) {
            if (it->borrowedBooks.size() > 0) { res.status = 400; res.set_content("Cannot delete: Has books.", "text/plain"); } 
            else { registered_users.erase(it); SaveUsers(); res.status = 200; }
        }
    });

    std::cout << "Server starting on http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
}