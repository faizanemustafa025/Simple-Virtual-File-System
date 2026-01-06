# Simple Virtual File System 📂

**Course:** Object-Oriented Data Structures (CL2021)  
**Language:** C++  

## 📖 Project Overview
This project simulates a low-level **Virtual File System (VFS)** using a single 10 MB binary container. It demonstrates core Operating System concepts including memory management, block allocation, and file metadata handling without relying on high-level file stream libraries.

The system is architected into three distinct regions:
1.  **Metadata Region:** Uses a **Hash Table** for O(1) file lookups.
2.  **Free Block Map:** A vector-based list to track available storage.
3.  **Data Region:** 8 MB of storage managed via **Linked Block Allocation**.

## 🚀 Key Features
* **File Management:** Create, View, Delete, and List files.
* **Modification:** Append-only logic to simulate writing to end-of-file.
* **Import/Export:** Transfer `.txt` files between the host Windows OS and the Virtual Disk.
* **Defragmentation:** Reorganizes blocks to contiguous memory to optimize read speeds.
* **Persistence:** All data is stored in a single `FileSystem.bin` file.

## 🛠️ Data Structures Used
* **Hash Table:** Used for the Directory structure to ensure efficient file search and insertion.
* **Linked List (Simulated):** Used for Data Blocks, where each 1024-byte block contains a pointer to the next block in the chain.
* **Vectors:** Used for managing the free list in memory.

## ⚙️ How to Run
1.  Clone the repository.
2.  Open `main.cpp` in Visual Studio or any C++ compiler.
3.  Compile and Run.
4.  The program will automatically generate the `FileSystem.bin` container (10 MB) on the first run.

## 📄 Documentation
For a deep dive into the system architecture, flow charts, and UML diagrams, please refer to the [Project Report](Project_Report.pdf) included in this repository.

---
*Created by Faizan e Mustafa & Hammad Ahmad | FAST-NUCES*
