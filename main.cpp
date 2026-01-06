#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <limits>

using namespace std;

// ---------------- Global Constants ----------------
const int BLOCK_SIZE = 1024;                 // 1 KB per block
const int TOTAL_SIZE = 10 * 1024 * 1024;     // 10 MB total
const int DIR_SIZE   = 1 * 1024 * 1024;      // 1 MB directory
const int FREE_SIZE  = 1 * 1024 * 1024;      // 1 MB free list
const int DATA_SIZE  = 8 * 1024 * 1024;      // 8 MB data region
const int NUM_BLOCKS = DATA_SIZE / BLOCK_SIZE; // 8192 blocks

// ---------------- Directory Entry ----------------
struct DirectoryEntry
{
    string filename;
    int startBlock;
    int fileSize;
    bool occupied;
};

// ---------------- Hash Table ----------------
class HashTable
{
public:
    vector<DirectoryEntry> table;
    int capacity;

    HashTable(int size)
    {
        capacity = size;
        table.resize(size);
        for (auto &entry : table)
            entry.occupied = false;
    }

    int hashFunction(const string &key)
    {
        unsigned long hash = 0;
        for (char c : key)
            hash = (hash * 31 + c) % capacity;
        return static_cast<int>(hash);
    }

    bool insert(const string &filename, int startBlock, int fileSize)
    {
        int idx = hashFunction(filename);
        int originalIdx = idx;

        while (table[idx].occupied)
        {
            if (table[idx].filename == filename)
                return false;

            idx = (idx + 1) % capacity;
            if (idx == originalIdx)
                return false;
        }

        table[idx] = {filename, startBlock, fileSize, true};
        return true;
    }

    DirectoryEntry* search(const string &filename)
    {
        int idx = hashFunction(filename);
        int originalIdx = idx;

        while (table[idx].occupied)
        {
            if (table[idx].filename == filename)
                return &table[idx];

            idx = (idx + 1) % capacity;
            if (idx == originalIdx)
                break;
        }
        return nullptr;
    }

    bool remove(const string &filename)
    {
        DirectoryEntry* entry = search(filename);
        if (entry)
        {
            entry->occupied = false;
            return true;
        }
        return false;
    }

    void listFiles()
    {
        int count = 1;
        for (auto &entry : table)
        {
            if (entry.occupied)
                cout << count++ << ". " << entry.filename << " (Size: " << entry.fileSize << " bytes)" << endl;
        }
    }

    bool isEmpty() const
    {
        for (const auto &entry : table)
            if (entry.occupied)
                return false;
        return true;
    }
};

// ---------------- File System ----------------
class FileSystem
{
private:
    string containerFile;
    HashTable directory;
    vector<int> freeList;

public:
    FileSystem(const string &filename)
        : directory(NUM_BLOCKS)
    {
        containerFile = filename;
        initialize();
    }

    void initialize()
    {
        if (!filesystem::exists(containerFile))
        {
            cout << "Creating new file system container...\n";

            ofstream ofs(containerFile, ios::binary);
            vector<char> buffer(TOTAL_SIZE, 0);
            ofs.write(buffer.data(), buffer.size());
            ofs.close();

            freeList.clear();
            for (int i = 0; i < NUM_BLOCKS; i++)
                freeList.push_back(i);
        }
        else
        {
            cout << "Loading existing file system...\n";

            directory = HashTable(NUM_BLOCKS);
            freeList.clear();
            for (int i = 0; i < NUM_BLOCKS; i++)
                freeList.push_back(i);
        }
    }

    void listFiles()
    {
        if (directory.isEmpty())
            cout << "No files exist in the system.\n";
        else
        {
            cout << "\nFiles in system:\n";
            directory.listFiles();
        }
    }

    void writeBlock(int blockIndex, const string &data, int nextBlock)
    {
        fstream fs(containerFile, ios::in | ios::out | ios::binary);

        int offset = DIR_SIZE + FREE_SIZE + blockIndex * BLOCK_SIZE;

        char buffer[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE);

        strncpy(buffer, data.c_str(), BLOCK_SIZE - sizeof(int));
        memcpy(buffer + (BLOCK_SIZE - sizeof(int)), &nextBlock, sizeof(int));

        fs.seekp(offset);
        fs.write(buffer, BLOCK_SIZE);
        fs.close();
    }

    string readBlock(int blockIndex, int &nextBlock)
    {
        fstream fs(containerFile, ios::in | ios::binary);

        int offset = DIR_SIZE + FREE_SIZE + blockIndex * BLOCK_SIZE;

        char buffer[BLOCK_SIZE];
        fs.seekg(offset);
        fs.read(buffer, BLOCK_SIZE);
        fs.close();

        memcpy(&nextBlock, buffer + (BLOCK_SIZE - sizeof(int)), sizeof(int));

        size_t len = 0;
        while (len < BLOCK_SIZE - sizeof(int) && buffer[len] != '\0')
            len++;

        return string(buffer, len);
    }

    void createFile()
    {
        cin.ignore();
        string filename, data;
        
        cout << "Enter filename: ";
        getline(cin, filename);
        
        if (directory.search(filename))
        {
            cout << "Error: File already exists!\n";
            return;
        }
        
        cout << "Enter file content: ";
        getline(cin, data);
        
        int dataPerBlock = BLOCK_SIZE - sizeof(int);
        int blocksNeeded = (data.size() + dataPerBlock - 1) / dataPerBlock;
        
        if (blocksNeeded > freeList.size())
        {
            cout << "Error: Not enough space!\n";
            return;
        }
        
        vector<int> allocatedBlocks;
        for (int i = 0; i < blocksNeeded; i++)
        {
            allocatedBlocks.push_back(freeList.back());
            freeList.pop_back();
        }
        
        for (int i = 0; i < allocatedBlocks.size(); i++)
        {
            int start = i * dataPerBlock;
            int end = min((i + 1) * dataPerBlock, (int)data.size());
            string chunk = data.substr(start, end - start);
            
            int nextBlock = (i == allocatedBlocks.size() - 1) ? -1 : allocatedBlocks[i + 1];
            writeBlock(allocatedBlocks[i], chunk, nextBlock);
        }
        
        directory.insert(filename, allocatedBlocks[0], data.size());
        cout << "File created successfully!\n";
    }

    void viewFile()
    {
        if (directory.isEmpty())
        {
            cout << "No files exist in the system.\n";
            return;
        }
        
        listFiles();
        
        cin.ignore();
        string filename;
        cout << "\nEnter filename to view: ";
        getline(cin, filename);
        
        DirectoryEntry* entry = directory.search(filename);
        if (!entry)
        {
            cout << "Error: File not found!\n";
            return;
        }
        
        cout << "\n--- Content of '" << filename << "' ---\n";
        
        int currentBlock = entry->startBlock;
        string content;
        
        while (currentBlock != -1)
        {
            int nextBlock;
            string chunk = readBlock(currentBlock, nextBlock);
            content += chunk;
            currentBlock = nextBlock;
        }
        
        cout << content << endl;
        cout << "--- End of file ---\n";
    }

    void deleteFile()
    {
        cin.ignore();
        string filename;
        
        cout << "Enter filename to delete: ";
        getline(cin, filename);
        
        DirectoryEntry* entry = directory.search(filename);
        if (!entry)
        {
            cout << "Error: File not found!\n";
            return;
        }
        
        int currentBlock = entry->startBlock;
        
        while (currentBlock != -1)
        {
            int nextBlock;
            readBlock(currentBlock, nextBlock);
            freeList.push_back(currentBlock);
            currentBlock = nextBlock;
        }
        
        directory.remove(filename);
        cout << "File deleted successfully!\n";
    }

    void modifyFile()
    {
        cin.ignore();
        string filename, extraData;
        
        cout << "Enter filename to modify: ";
        getline(cin, filename);
        
        DirectoryEntry* entry = directory.search(filename);
        if (!entry)
        {
            cout << "Error: File not found!\n";
            return;
        }
        
        cout << "Enter data to append: ";
        getline(cin, extraData);
        
        string existingContent;
        int currentBlock = entry->startBlock;
        
        while (currentBlock != -1)
        {
            int nextBlock;
            string chunk = readBlock(currentBlock, nextBlock);
            existingContent += chunk;
            currentBlock = nextBlock;
        }
        
        deleteFile();
        
        string newContent = existingContent + extraData;
        int dataPerBlock = BLOCK_SIZE - sizeof(int);
        int blocksNeeded = (newContent.size() + dataPerBlock - 1) / dataPerBlock;
        
        if (blocksNeeded > freeList.size())
        {
            cout << "Error: Not enough space for modification!\n";
            return;
        }
        
        vector<int> allocatedBlocks;
        for (int i = 0; i < blocksNeeded; i++)
        {
            allocatedBlocks.push_back(freeList.back());
            freeList.pop_back();
        }
        
        for (int i = 0; i < allocatedBlocks.size(); i++)
        {
            int start = i * dataPerBlock;
            int end = min((i + 1) * dataPerBlock, (int)newContent.size());
            string chunk = newContent.substr(start, end - start);
            
            int nextBlock = (i == allocatedBlocks.size() - 1) ? -1 : allocatedBlocks[i + 1];
            writeBlock(allocatedBlocks[i], chunk, nextBlock);
        }
        
        directory.insert(filename, allocatedBlocks[0], newContent.size());
        cout << "File modified successfully!\n";
    }

    void copyFromWindows()
    {
        cin.ignore();
        string srcPath, destFilename;
        
        cout << "Enter source file path: ";
        getline(cin, srcPath);
        
        cout << "Enter destination filename: ";
        getline(cin, destFilename);
        
        if (directory.search(destFilename))
        {
            cout << "Error: File already exists in the system!\n";
            return;
        }
        
        ifstream srcFile(srcPath, ios::binary);
        if (!srcFile)
        {
            cout << "Error: Cannot open source file!\n";
            return;
        }
        
        string content((istreambuf_iterator<char>(srcFile)), istreambuf_iterator<char>());
        srcFile.close();
        
        int dataPerBlock = BLOCK_SIZE - sizeof(int);
        int blocksNeeded = (content.size() + dataPerBlock - 1) / dataPerBlock;
        
        if (blocksNeeded > freeList.size())
        {
            cout << "Error: Not enough space!\n";
            return;
        }
        
        vector<int> allocatedBlocks;
        for (int i = 0; i < blocksNeeded; i++)
        {
            allocatedBlocks.push_back(freeList.back());
            freeList.pop_back();
        }
        
        for (int i = 0; i < allocatedBlocks.size(); i++)
        {
            int start = i * dataPerBlock;
            int end = min((i + 1) * dataPerBlock, (int)content.size());
            string chunk = content.substr(start, end - start);
            
            int nextBlock = (i == allocatedBlocks.size() - 1) ? -1 : allocatedBlocks[i + 1];
            writeBlock(allocatedBlocks[i], chunk, nextBlock);
        }
        
        directory.insert(destFilename, allocatedBlocks[0], content.size());
        cout << "File copied successfully from Windows!\n";
    }

    void copyToWindows()
    {
        cin.ignore();
        string filename, destPath;
        
        cout << "Enter filename to copy: ";
        getline(cin, filename);
        
        DirectoryEntry* entry = directory.search(filename);
        if (!entry)
        {
            cout << "Error: File not found!\n";
            return;
        }
        
        cout << "Enter destination path: ";
        getline(cin, destPath);
        
        string content;
        int currentBlock = entry->startBlock;
        
        while (currentBlock != -1)
        {
            int nextBlock;
            string chunk = readBlock(currentBlock, nextBlock);
            content += chunk;
            currentBlock = nextBlock;
        }
        
        ofstream destFile(destPath, ios::binary);
        if (!destFile)
        {
            cout << "Error: Cannot create destination file!\n";
            return;
        }
        
        destFile << content;
        destFile.close();
        
        cout << "File copied successfully to Windows!\n";
    }

    void defragmentation()
    {
        cout << "Starting defragmentation...\n";
        
        vector<pair<string, string>> fileContents;
        
        for (auto &entry : directory.table)
        {
            if (entry.occupied)
            {
                string content;
                int currentBlock = entry.startBlock;
                
                while (currentBlock != -1)
                {
                    int nextBlock;
                    string chunk = readBlock(currentBlock, nextBlock);
                    content += chunk;
                    currentBlock = nextBlock;
                }
                
                fileContents.push_back({entry.filename, content});
            }
        }
        
        directory = HashTable(NUM_BLOCKS);
        freeList.clear();
        for (int i = 0; i < NUM_BLOCKS; i++)
            freeList.push_back(i);
        
        reverse(freeList.begin(), freeList.end());
        
        for (const auto &filePair : fileContents)
        {
            const string &filename = filePair.first;
            const string &content = filePair.second;
            
            int dataPerBlock = BLOCK_SIZE - sizeof(int);
            int blocksNeeded = (content.size() + dataPerBlock - 1) / dataPerBlock;
            
            vector<int> allocatedBlocks;
            for (int i = 0; i < blocksNeeded; i++)
            {
                allocatedBlocks.push_back(freeList.back());
                freeList.pop_back();
            }
            
            for (int i = 0; i < allocatedBlocks.size(); i++)
            {
                int start = i * dataPerBlock;
                int end = min((i + 1) * dataPerBlock, (int)content.size());
                string chunk = content.substr(start, end - start);
                
                int nextBlock = (i == allocatedBlocks.size() - 1) ? -1 : allocatedBlocks[i + 1];
                writeBlock(allocatedBlocks[i], chunk, nextBlock);
            }
            
            directory.insert(filename, allocatedBlocks[0], content.size());
        }
        
        cout << "Defragmentation completed!\n";
    }
};

// ---------------- Main ----------------
int main()
{
    FileSystem fs("File_system.bin");

    int choice;

    while (true)
    {
        cout << "\n===== File System Menu =====\n";
        cout << "1. Create New File\n";
        cout << "2. List & View Existing Files\n";
        cout << "3. Modify File (Append Only)\n";
        cout << "4. Delete File\n";
        cout << "5. Copy File from Windows\n";
        cout << "6. Copy File to Windows\n";
        cout << "7. Defragmentation\n";
        cout << "8. Exit\n";
        cout << "Enter your choice: ";

        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number.\n";
            continue;
        }

        switch (choice)
        {
            case 1:
                fs.createFile();
                break;
            case 2:
                fs.viewFile();
                break;
            case 3:
                fs.modifyFile();
                break;
            case 4:
                fs.deleteFile();
                break;
            case 5:
                fs.copyFromWindows();
                break;
            case 6:
                fs.copyToWindows();
                break;
            case 7:
                fs.defragmentation();
                break;
            case 8:
                cout << "Exiting file system. Goodbye!\n";
                return 0;
            default:
                cout << "Invalid choice. Please try again.\n";
        }
    }

    return 0;
}
