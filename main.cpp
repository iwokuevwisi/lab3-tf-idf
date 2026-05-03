/***
TODO
1. Заменить while
2. Чтение файлов из нескольких слов

***/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <cctype>
#include <iterator>
#include <set>
#include <cmath>
#include <ranges>
#include <format>


// Структуры
struct Word {
    double idf = 0;  
    std::set<std::string> docs; // Документы, в которых встретилось слово
};

struct WordStats {
    int count = 0; // Кол-во вхождений слова в документ
    double tf = 0;
    double tf_idf = 0;
};

struct Doc {
    int total_words = 0; // Всего слов в документе
    std::unordered_map<std::string, WordStats> unique_words; // Уникальные слова в документе
};


// Константы
int DOCS_COUNT = 0; // Всего документов

std::unordered_map<std::string, Word> DICTIONARY; // Словарь всех слов
std::unordered_map<std::string, Doc> DOCS; // Коллекция всех документов


// Функции для обработки запросов
void queryWord(const std::string& word);
void queryWordInDoc(const std::string& word, const std::string& doc_name);
void queryDoc(const std::string& doc_name);
void queryWordsCollection(const std::vector<std::string>& words);


// Функции-алгоритмы
void processDocuments(std::ifstream& fin);
std::string cleanText(const std::string& original_text); 
std::vector<std::string> tokenizeText(const std::string& text);


int main() {
    std::ifstream fin("documents.txt");
    if (!fin.is_open()) {
        std::cerr << "Error reading 'documents.txt' file";
        std::exit(EXIT_FAILURE);
    }

    processDocuments(fin);

    std::string query;
    while (std::getline(std::cin, query)) {
        std::vector<std::string> query_tokens = tokenizeText(query);
        if (query_tokens.size() == 2 && query_tokens[0] == "WORD") {
            queryWord(cleanText(query_tokens[1]));
        } else if (query_tokens.size() == 3 && query_tokens[0] == "WORD_IN_DOC") {
            queryWordInDoc(cleanText(query_tokens[1]), query_tokens[2]);
        } else if (query_tokens.size() == 2 && query_tokens[0] == "DOC") {
            queryDoc(query_tokens[1]);
        } else if (query_tokens.size() > 1 && query_tokens[0] == "QUERY") {
            std::ranges::for_each(query_tokens | std::views::drop(1), [&](auto& word) {
                word = cleanText(word);
            });
            queryWordsCollection({query_tokens.begin() + 1, query_tokens.end()});
        } else {
            std::cout << "This query not found\n";
        }
    }
}

void queryWord(const std::string& word) {
    if (!DICTIONARY.contains(word)) {
        std::cout << std::format("Word {} not found\n", word);
        return;
    }
    const auto& word_stats = DICTIONARY.at(word);

    std::cout << std::format("Word: {}\n", word);
    std::cout << std::format("Documents total: {}\n", DOCS_COUNT);
    std::cout << std::format("Documents with words: {}\n", word_stats.docs.size());
    std::cout << std::format("IDF: {:.4f}\n", word_stats.idf);
    std::cout << std::format("Appears in:\n");
    std::ranges::for_each(word_stats.docs, [&](const auto& doc_name) {
        std::cout << std::format("- {}\n", doc_name);
    });
}

void queryWordInDoc(const std::string& word, const std::string& doc_name) {
    if (!DOCS.contains(doc_name)) {
        std::cout << std::format("Document {} not found\n", doc_name);
        return;
    }
    const auto& doc = DOCS.at(doc_name);

    if (!doc.unique_words.contains(word)) {
        std::cout << std::format("Word {} not found in this document\n", word);
        return;
    }
    const auto& word_stats = doc.unique_words.at(word);

    std::cout << std::format("Word: {}\n", word);
    std::cout << std::format("Document: {}\n", doc_name);
    std::cout << std::format("Count: {}\n", word_stats.count);
    std::cout << std::format("TF: {:.4f}\n", word_stats.tf);
    std::cout << std::format("TF-IDF: {:.4f}\n", word_stats.tf_idf);
}

void queryDoc(const std::string& doc_name) {
    if (!DOCS.contains(doc_name)) {
        std::cout << "This document not found\n";
        return;
    }
    const auto& doc = DOCS.at(doc_name);

    // Сортируем с помощью partial_sort, предварительно закинув unordered_map в вектор пар
    std::vector<std::pair<std::string, WordStats>> words(doc.unique_words.begin(), doc.unique_words.end());
    int n = std::min(5, static_cast<int> (words.size()));
    std::partial_sort(words.begin(), words.begin() + n, words.end(), [](const auto& word_1, const auto& word_2){
        return word_1.second.tf_idf > word_2.second.tf_idf;
    });
    auto top_5 = words | std::views::take(5);

    std::cout << std::format("Document: {}\n", doc_name);
    std::cout << std::format("Total words: {}\n", doc.total_words);
    std::cout << std::format("Unique words: {}\n", doc.unique_words.size());
    std::cout << std::format("Top words:\n");
    int cnt = 1;
    std::ranges::for_each(top_5, [&](const auto& item) {
        const auto& [word, stats] = item;
        std::cout << std::format("{}. {} ({:.4f})\n", cnt++, word, stats.tf_idf);
    });
}

void queryWordsCollection(const std::vector<std::string>& words) {
    std::unordered_map<std::string, double> doc_scores;
    
    // Считаем сумму TF-IDF для каждого документа
    std::ranges::for_each(words, [&](const auto& word) {
        // Если слово пустое или нет в словаре
        if (word.empty() || !DICTIONARY.contains(word)) {
            return;
        }

        std::ranges::for_each(DICTIONARY.at(word).docs, [&](const auto& doc_name) {
            doc_scores[doc_name] += DOCS.at(doc_name).unique_words.at(word).tf_idf;
        });
    });

    // Не найдено ни одно слово
    if (doc_scores.empty()) {
        std::cout << "No documents found\n";
        return;
    }

    std::vector<std::pair<std::string, double>> results(doc_scores.begin(), doc_scores.end());
    std::ranges::sort(results, [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    auto top_3 = results | std::views::take(3);
    
    std::cout << "Query:";
    std::ranges::for_each(words, [](const auto& word) {
        std::cout << " " << word;
    });
    std::cout << "\nResults:\n";
    
    int cnt = 1;
    std::ranges::for_each(top_3, [&](const auto& item) {
        const auto& [doc_name, tf_idf] = item;
        std::cout << std::format("{}. {} ({:.4f})\n", cnt++, doc_name, tf_idf);
    });
}

void processDocuments(std::ifstream& fin) {
    std::string doc_name;

    // Открываем каждый документ и считаем TF для слов в каждом документе
    while (std::getline(fin, doc_name)) {
        std::ifstream fin_doc(doc_name);
        if (!fin_doc.is_open()) {
            std::cout << "Error loading " << doc_name << ".txt file\n";
            continue;
        }
        DOCS_COUNT++;

        // Считываем весь текст из документа, чистим текст, разбиваем на отдельные слова
        std::string original_text{std::istreambuf_iterator<char> {fin_doc},
                                  std::istreambuf_iterator<char> {}};
        std::string clean_text = cleanText(original_text);
        std::vector<std::string> tokens = tokenizeText(clean_text);

        // Считаем уникальные слова и кол-во вхождений в документе
        std::unordered_map<std::string, WordStats> unique_words;
        std::for_each(tokens.begin(), tokens.end(), [&](const auto& token) {
            unique_words[token].count++;
        });

        Doc doc = {(int) tokens.size(), unique_words};
        DOCS[doc_name] = doc;

        // Добавляем наименование документа, в котором данное слово фигурировало
        std::ranges::for_each(unique_words, [&](const auto& item) {
            const auto& [token, word_stats] = item;
            // Если слова не было в словаре, то создастся такой элемент.
            // Иначе, получим ссылку на него
            auto& word = DICTIONARY[token];
            word.docs.insert(doc_name);
        });
        /* Альтернативный вариант того же самого:
        for (const auto& [token, word_stats] : unique_words) {
            auto& word = DICTIONARY[token];
            word.docs.insert(doc_name);
        }
        */
    }

    // Расчет IDF для каждого слова
    std::ranges::for_each(DICTIONARY, [](auto& item){
        auto& [word, word_data] = item;
        word_data.idf = std::log(static_cast<double>(DOCS_COUNT) / word_data.docs.size());
    });

    // Расчет TF, TF-IDF для каждого слова в каждом документе
    std::ranges::for_each(DOCS, [&](auto& doc_item){
        auto& [doc_name, doc_stats] = doc_item;
        std::ranges::for_each(doc_stats.unique_words, [&](auto& word_item) {
            auto& [word_name, word_stats] = word_item;
            word_stats.tf = static_cast<double>(word_stats.count) / doc_stats.total_words;
            word_stats.tf_idf = word_stats.tf * DICTIONARY[word_name].idf;
        });
    });
}

// Очищаем от знаков пунктуации, управляющих символов и символов с ASCII > 127
std::string cleanText(const std::string& original_text) {
    std::string clean_text;
    clean_text.reserve(original_text.size());
    std::transform(original_text.begin(), 
                   original_text.end(), 
                   std::back_inserter(clean_text), 
                   [](unsigned char ch) {
                      if (std::ispunct(ch) || std::iscntrl(ch) || ch > 127) {
                          return (unsigned char) ' ';   
                      } else {
                          return (unsigned char) std::tolower(ch);
                      }
                    });
    // std::erase_if(clean_text, 
    //               [](unsigned char ch) {return std::ispunct(ch) || std::iscntrl(ch) ||ch > 127;});
    return clean_text;
}

// Разбиваем текст на вектор слов по пробелам
std::vector<std::string> tokenizeText(const std::string& text) {
    std::stringstream ss(text);
    auto tokens = std::ranges::istream_view<std::string>(ss) | std::ranges::to<std::vector<std::string>>(); 
    return tokens;

    /* Альтернативный вариант (До C++20)
    std::stringstream ss(text);
    std::vector<std::string> tokens = {std::istream_iterator<std::string> (ss),
                                       std::istream_iterator<std::string> ()};
    return tokens;
    */
}
