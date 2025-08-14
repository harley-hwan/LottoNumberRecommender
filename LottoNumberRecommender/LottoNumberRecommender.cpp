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
#include <numeric>  // std::iota�� ���� �߰�
#include <chrono>   // �ð� ������ ���� �߰�

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
    std::cout << "    �ζ� 6/45 ��ȣ ��õ ���α׷� ����" << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "\n���ູ�� ���� API���� �����͸� �������� ���Դϴ�..." << std::endl;
    std::cout << "��Ʈ��ũ ���¿� ���� �ð��� �ɸ� �� �ֽ��ϴ�.\n" << std::endl;

    std::array<int, 46> freq = { 0 }; // Index 1 to 45

    int drwNo = 1;
    int total_draws = 0;

    // ���� �ð� ���
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

        // ���� ��Ȳ ǥ�� (50ȸ������)
        if (drwNo % 50 == 0) {
            std::cout << "������... " << drwNo << "ȸ������ �м� �Ϸ�" << std::endl;
        }
        // ó�� �� ȸ���� �ٷ� ǥ���Ͽ� ���α׷��� ���� ������ �˸�
        else if (drwNo <= 5) {
            std::cout << drwNo << "ȸ�� ������ ����..." << std::endl;
        }

        ++drwNo;
    }

    // ��� �ð� ���
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

    std::cout << "\n������ ���� �Ϸ�!" << std::endl;
    std::cout << "�� " << total_draws << "ȸ�� �����͸� " << duration << "�� ���� �����߽��ϴ�." << std::endl;
    std::cout << "\n��ȣ�� ���� �󵵸� �м� ���Դϴ�..." << std::endl;

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

    std::cout << "\n�м� �Ϸ�! ��õ ��ȣ ������ ���� ���Դϴ�...\n" << std::endl;

    // Output the mandatory lowest frequency combination first
    std::cout << "===========================================" << std::endl;
    std::cout << "�ζ� 6/45 ��ȣ ��õ (���� ���� �� �켱 ���� ���� ���� 10��Ʈ):" << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "���� 1 : ";
    for (size_t i = 0; i < lowest_combo.size(); ++i) {
        std::cout << lowest_combo[i];
        if (i < lowest_combo.size() - 1) std::cout << ", ";
    }
    std::cout << " (���� ���� �� 6�� ���� ����)" << std::endl;

    // Generate additional 9 combinations
    std::cout << "\n�߰� ���� ..." << std::endl;
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
        std::cout << "���� " << (combo + 1) << ": ";
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
    std::cout << "              �м� ��� ���" << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "ó���� �� ȸ�� ��: " << total_draws << "ȸ" << std::endl;
    std::cout << "�� ���� Ƚ�� (����, ����: " << total_draws * 6 << "): " << total_appearances << std::endl;
    std::cout << "������ ��ó: ���ູ�� ���� API (�ǽð� fetch)" << std::endl;
    std::cout << "===========================================" << std::endl;

    // ���α׷��� �ٷ� ������� �ʵ��� ����� �Է� ���
    std::cout << "\n�����Ϸ��� Enter Ű�� ��������..." << std::endl;
    std::cin.ignore();
    std::cin.get();

    return 0;
}