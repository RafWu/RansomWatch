#pragma once

#include "ExtensionsCategories.h"

// minimum files and dirs required in protected area to check malicious
constexpr int MINIMUM_DIRS_THRESHOLD = 10;
constexpr int MINIMUM_FILES_THRESHOLD = 30;

// enable malicious check if process starts to write high entropy files
constexpr int NUM_WRITES_FOR_TRIGGER = 5;

// Triggers required for malicious
constexpr char TRIGGERS_TRESHOLD = 8; // and if found NUM_WRITES_FOR_TRIGGER writes with high entropy

///
constexpr int ENTROPY_FILES_INCREASED_THRESHOLD = 20;

// Access
constexpr double ACCESS_FILES_TRESHOLDS			= 0.45;	// accessed to write out of read
constexpr int	 MINIMUM_FILES_ACCESS_THRESHOLD	= 50;	// from accessed files

// Create
constexpr double FILES_CREATED_THRESHOLD		= 0.49;	// number of files protected vs number of files accessed
constexpr int	 MINIMUM_FILES_CREATE_THRESHOLD	= 25;	// minimum files created

// Delete
constexpr double FILES_DELETED_THRESHOLD		= 0.2;	// from total protected
constexpr double DELETED_ACCESSED_THRESHOLD		= 0.3;	// from accessed files

// Entropy
constexpr double MAX_ENTROPY = 8.0;
constexpr double HIGH_ENTROPY_THRESHOLD			= 7.6;

// Extension
constexpr double FILES_EXTENSION_THRESHOLD		= 0.25;	// diff between number of extension written to those read and written

// Extension change
constexpr double CHANGE_EXTENSION_THRESHOLD		= 0.2;	// change extension out of accessed files

// Extension sensitive
constexpr double EXTENSION_OPENED_SENSITIVE		= 0.25;	// number of different categories that the application opened out of NUM_CATEGORIES_WITH_OTHERS

// Listing
constexpr double LISTING_THRESHOLD				= 0.4;	// listed directories out of all subdirs in protected area

// Moving
constexpr double MOVE_IN_THRESHOLD				= 0.05;	// listed directories out of all subdirs in protected area
constexpr double MOVE_OUT_THRESHOLD				= 0.1;	// listed directories out of all subdirs in protected area

// Read
constexpr double FILES_READ_THRESHOLD			= 0.4;	// files read out of all files in protected area

// Rename
constexpr double FILES_RENAMED_THRESHOLD		= 0.2;	// from total protected
constexpr double RENAMED_ACCESSED_THRESHOLD		= 0.3;	// from accessed files

//Traps
constexpr int	 TRAP_WEIGHT					= 9;	// extra weight to action in case of traps
constexpr int	 TRAPS_DIRS_TRIGGER_THRESHOLD	= 4;	// different dirs traps changed/read
//constexpr int	 MINIMUM_TRAPS_TRIGGER_TRESHOLD = 50;		// different traps actions

