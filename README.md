# Library Management System

## About the Project
The Library Management System is a comprehensive web application designed to digitize and streamline everyday library operations. It provides a seamless, interactive interface for students to browse book catalogs, manage their active loans, and track outstanding fines. Simultaneously, it equips library administrators with a secure dashboard to manage inventory, register new users, and enforce library policies. 

## Key Features
* **Role-Based Dashboards (RBAC):** Distinct, secure interfaces for Students and Librarians. Students can view personal borrowing histories, while Librarians have access to global library statistics and user management.
* **Automated Fine & Time Tracking:** Integrates a custom time-engine using Unix epoch time (`mktime`, `difftime`) to track borrow durations, enforce loan limits, and automatically calculate overdue fines.
* **Administrative Control:** Full CRUD (Create, Read, Update, Delete) operations allow Librarians to easily add new books to the catalog or safely remove users.

## Technical Architecture
* **Backend Engine:** Native C++ RESTful API
* **Frontend UI:** React.js Single Page Application (SPA) built with Vite
* **Networking & Routing:** `cpp-httplib` for handling HTTP requests and `nlohmann/json` for data serialization.
* **Authentication:** $O(1)$ token-based, in-memory session management utilizing `std::unordered_map`, with memory-safe object allocation via `std::list` to prevent pointer invalidation.
* **Data Persistence:** Custom C++ File I/O system managing state across server reboots via delimited text files.

## How to Run Locally

### 1. Start the C++ Backend Server
Navigate to the build directory, compile, and run the executable:
```bash
cd Backend/build
cmake --build .
./server.exe
