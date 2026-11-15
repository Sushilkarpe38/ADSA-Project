@echo off
echo [*] Compiling ReFind Phase 5 (with Custom Max-Heap) from src...
g++ -std=c++17 -O2 -I../include ItemRecord.cpp Normalizer.cpp CategoryIndex.cpp Trie.cpp CandidateGenerator.cpp Scorer.cpp MaxHeap.cpp demo_phase5.cpp -o demo_phase5.exe
if %ERRORLEVEL% EQU 0 (
    copy demo_phase5.exe ..\demo_phase5.exe >nul
    echo [+] Compilation Successful! Created demo_phase5.exe
) else (
    echo [!] Compilation Failed.
)
