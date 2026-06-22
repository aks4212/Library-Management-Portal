# Full-Stack Library Management System

A decoupled client-server web application transitioning a monolithic C++ object-oriented system into a modern web architecture. 

## Technical Architecture
* **Backend:** Native C++ RESTful API
* **Frontend:** React.js Single Page Application (SPA) utilizing Vite
* **Networking:** `cpp-httplib` for network routing and `nlohmann/json` for API data serialization
* **Authentication:** $O(1)$ token-based, in-memory session management utilizing `std::unordered_map` and memory-safe object allocation via `std::list`.
* **Data Persistence:** Custom C++ File I/O system reading/writing to delimited text files.

## Features
* **Role-Based Access Control (RBAC):** Distinct dashboards and permissions for Students and Librarians.
* **Complex Transaction Logic:** Automated time-based fine calculations using Unix epoch time (`mktime`, `difftime`).
* **Admin Tools:** Full CRUD operations for Librarians to manage inventory and user accounts safely.
* **Time Travel Simulator:** Frontend testing tool to simulate future dates to test backend overdue logic.

## How to Run Locally

### 1. Start the C++ Backend
```bash
cd Backend/build
cmake --build .
./server.exe
