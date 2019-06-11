#pragma once

constexpr double LISTING_THRESHOLD = 0.4;
constexpr int EXTENSIONS_WRITE_THRESHOLD = 6; // what about read
constexpr double RENAMED_THRESHOLD = 0.2;
constexpr double FILES_READ_THRESHOLD = 0.4;
constexpr double FILES_CREATED_THRESHOLD = 0.1;
constexpr double FILES_WRITTEN_THRESHOLD = 0.2;
constexpr double FILES_DELETED_THRESHOLD = 0.1;
constexpr double ENTROPY_THRESHOLD = 7.4;
constexpr long TRAP_WEIGHT = 10;
constexpr int MINIMUM_DIRS_THRESHOLD = 10;
constexpr int MINIMUM_FILES_THRESHOLD = 30;

constexpr int NUM_WRITES_FOR_TRIGGER = 20;
constexpr char TRIGGERS_TRESHOLD = 4; // and if found MINIMUM_FILES_THRESHOLD writes with high entropy

///
constexpr int ENTROPY_FILES_INCREASED_THRESHOLD = 20;
constexpr double ENTROPY_INCREASED_THRESHOLD = 1.2;