#include <iostream>

namespace LinearDemo { void Run(); }
namespace StackDemo { void Run(); }
namespace PoolDemo { void Run(); }
namespace BuddyDemo { void Run(); }
namespace BenchmarkRunner { void RunAll(); }
namespace GameSimDemo { void Run(); }

int main() {
    int choice = 0;
    while (true) {
        std::cout << "\n=== Memory Allocator Project ===\n"
            << "1. Linear Allocator Demo\n"
            << "2. Stack Allocator Demo\n"
            << "3. Pool Allocator Demo\n"
            << "4. Buddy Allocator Demo\n"
            << "5. Run All Benchmarks\n"
            << "6. Run 'Game' Demo\n"
            << "0. Exit\n"
            << "Choice: ";
        std::cin >> choice;
        switch (choice) {
        case 1: LinearDemo::Run();         break;
        case 2: StackDemo::Run();          break;
        case 3: PoolDemo::Run();           break;
        case 4: BuddyDemo::Run();          break;
        case 5: BenchmarkRunner::RunAll(); break;
        case 6: GameSimDemo::Run();        break;
        case 0: return 0;
        default: std::cout << "Invalid choice\n";
        }
    }
}