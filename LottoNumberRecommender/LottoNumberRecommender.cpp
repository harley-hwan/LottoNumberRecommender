// LottoNumberRecommender.cpp
// This program fetches historical Lotto 6/45 winning numbers from the official API,
// calculates frequencies, includes the combination of the 6 lowest frequency numbers,
// and recommends additional 9 random combinations of 6 numbers each,
// preferring numbers with lower appearance frequencies via weighted random selection.
// Compiled and tested with Visual Studio 2022 using C++17 standard.
// Requires linking with winhttp.lib (Project Properties > Linker > Input > Additional Dependencies: winhttp.lib)
// Author: Grok (AI-assisted code generation)

#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <array>
#include <vector>
#include <algorithm>
#include <string>
#include <regex>
#include <sstream>
#include <random>
#include <set>
#include <numeric>  // std::iota를 위해 추가
#include <chrono>   // 시간 측정을 위해 추가

#pragma comment(lib, "winhttp.lib")

std::string FetchLottoData(int drwNo) {
    std::wstring path = L"/common.do?method=getLottoNumber&drwNo=" + std::to_wstring(drwNo);

    HINTERNET hSession = WinHttpOpen(L"Lotto Fetcher", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";

    HINTERNET hConnect = WinHttpConnect(hSession, L"www.dhlottery.co.kr", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    std::string response;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    do {
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;

        if (dwSize == 0) break;

        std::vector<char> buffer(dwSize + 1);
        if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) break;

        response.append(buffer.data(), dwDownloaded);
    } while (dwSize > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return response;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "    로또 6/45 번호 추천 프로그램 시작" << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "\n동행복권 공식 API에서 데이터를 가져오는 중입니다..." << std::endl;
    std::cout << "네트워크 상태에 따라 시간이 걸릴 수 있습니다.\n" << std::endl;

    std::array<int, 46> freq = { 0 }; // Index 1 to 45

    int drwNo = 1;
    int total_draws = 0;

    // 시작 시간 기록
    auto start_time = std::chrono::steady_clock::now();

    while (true) {
        std::string json = FetchLottoData(drwNo);
        if (json.empty() || json.find("\"returnValue\":\"fail\"") != std::string::npos) {
            break;
        }

        // Parse winning numbers
        std::regex r(R"("drwtNo[1-6]":(\d+))");
        std::sregex_iterator iter(json.begin(), json.end(), r);
        std::sregex_iterator end;
        for (; iter != end; ++iter) {
            int num = std::stoi(iter->str(1));
            if (num >= 1 && num <= 45) {
                ++freq[num];
            }
        }

        ++total_draws;

        // 진행 상황 표시 (50회차마다)
        if (drwNo % 50 == 0) {
            std::cout << "진행중... " << drwNo << "회차까지 분석 완료" << std::endl;
        }
        // 처음 몇 회차는 바로 표시하여 프로그램이 동작 중임을 알림
        else if (drwNo <= 5) {
            std::cout << drwNo << "회차 데이터 수신..." << std::endl;
        }

        ++drwNo;
    }

    // 경과 시간 계산
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

    std::cout << "\n데이터 수집 완료!" << std::endl;
    std::cout << "총 " << total_draws << "회차 데이터를 " << duration << "초 만에 수집했습니다." << std::endl;
    std::cout << "\n번호별 출현 빈도를 분석 중입니다..." << std::endl;

    // Create vector of pairs: (frequency, number) for sorting lowest frequencies
    std::vector<std::pair<int, int>> num_freq_pairs;
    num_freq_pairs.reserve(45);
    for (int i = 1; i <= 45; ++i) {
        num_freq_pairs.emplace_back(freq[i], i);
    }

    // Sort by frequency ascending, then by number ascending
    std::sort(num_freq_pairs.begin(), num_freq_pairs.end());

    // The lowest 6 frequency numbers as the mandatory combination
    std::vector<int> lowest_combo;
    for (size_t i = 0; i < 6; ++i) {
        lowest_combo.push_back(num_freq_pairs[i].second);
    }
    std::sort(lowest_combo.begin(), lowest_combo.end());

    // Calculate max frequency
    int max_freq = 0;
    for (int i = 1; i <= 45; ++i) {
        if (freq[i] > max_freq) max_freq = freq[i];
    }

    // Weights: higher for lower frequencies (max_freq - freq + 1 to ensure positive)
    std::vector<double> base_weights(45);
    for (int i = 0; i < 45; ++i) {
        base_weights[i] = static_cast<double>(max_freq - freq[i + 1] + 1);
    }

    // Random generator
    std::mt19937 gen(std::random_device{}());

    std::cout << "\n분석 완료! 추천 번호 조합을 생성 중입니다...\n" << std::endl;

    // Output the mandatory lowest frequency combination first
    std::cout << "===========================================" << std::endl;
    std::cout << "로또 6/45 번호 추천 (낮은 출현 빈도 우선 가중 랜덤 조합 10세트):" << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "조합 1 : ";
    for (size_t i = 0; i < lowest_combo.size(); ++i) {
        std::cout << lowest_combo[i];
        if (i < lowest_combo.size() - 1) std::cout << ", ";
    }
    std::cout << " (가장 낮은 빈도 6개 숫자 조합)" << std::endl;

    // Generate additional 9 combinations
    std::cout << "\n추가 조합 ..." << std::endl;
    for (int combo = 1; combo < 10; ++combo) {
        std::vector<int> candidates(45);
        std::iota(candidates.begin(), candidates.end(), 1); // 1 to 45

        std::vector<double> curr_weights = base_weights; // Copy weights

        std::set<int> selected;
        while (selected.size() < 6) {
            std::discrete_distribution<int> dist(curr_weights.begin(), curr_weights.end());
            int idx = dist(gen);
            int num = candidates[idx];
            selected.insert(num);

            // Remove selected from candidates and weights
            candidates.erase(candidates.begin() + idx);
            curr_weights.erase(curr_weights.begin() + idx);
        }

        // Output sorted combination
        std::cout << "조합 " << (combo + 1) << ": ";
        for (auto it = selected.begin(); it != selected.end(); ++it) {
            std::cout << *it;
            if (std::next(it) != selected.end()) std::cout << ", ";
        }
        std::cout << std::endl;
    }

    // Verify
    int total_appearances = 0;
    for (int i = 1; i <= 45; ++i) {
        total_appearances += freq[i];
    }

    std::cout << "\n===========================================" << std::endl;
    std::cout << "              분석 결과 요약" << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "처리된 총 회차 수: " << total_draws << "회" << std::endl;
    std::cout << "총 출현 횟수 (검증, 예상: " << total_draws * 6 << "): " << total_appearances << std::endl;
    std::cout << "데이터 출처: 동행복권 공식 API (실시간 fetch)" << std::endl;
    std::cout << "===========================================" << std::endl;

    // 프로그램이 바로 종료되지 않도록 사용자 입력 대기
    std::cout << "\n종료하려면 Enter 키를 누르세요..." << std::endl;
    std::cin.ignore();
    std::cin.get();

    return 0;
}