@echo off
echo [*] Compiling ReFind Phase 5 (with Custom Max-Heap)...
g++ -std=c++17 -O2 -Iinclude src/ItemRecord.cpp src/Normalizer.cpp src/CategoryIndex.cpp src/Trie.cpp src/CandidateGenerator.cpp src/Scorer.cpp src/MaxHeap.cpp src/demo_phase5.cpp -o demo_phase5.exe
if %ERRORLEVEL% EQU 0 (
    copy demo_phase5.exe src\demo_phase5.exe >nul
    echo [+] Compilation Successful! Created demo_phase5.exe
) else (
    echo [!] Compilation Failed.
)
