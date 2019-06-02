#pragma once

#include <Windows.h>
#include <FltUser.h>
#include <comdef.h>
#include "../SharedDefs/SharedDefs.h"
#include <iostream>
#include "Common.h"
#include "TrapHandler.h"
#include "FilterWorker.h"

#pragma comment(lib, "fltMgr")
#pragma comment(lib, "fltLib")
#pragma comment(lib, "User32")
#pragma comment(lib, "kernel32")

SCANNER_THREAD_CONTEXT context;

namespace AntiRansomWareApp {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for MyForm
	/// </summary>

	public ref class MyForm : public System::Windows::Forms::Form
	{

	private: BOOLEAN dragging;
	private: Point offset;
	private: TrapHandler* trapHandler;
	private: System::Windows::Forms::CheckBox^  AutoKill;
	private: System::Windows::Forms::CheckBox^  ViewLog;
	private: System::Windows::Forms::CheckBox^  DisableProtection;
	private: System::Windows::Forms::Button^  SelectAddRootDir;
	private: System::Windows::Forms::Button^  SelectRemRootDir;
	private: System::Windows::Forms::Button^  InitTraps;
	private: System::Windows::Forms::TextBox^  logViewer;

	//public: HANDLE port;
	//public: HANDLE completion;
	private: HRESULT initWorkThread();
	private: BOOLEAN openKernelDriverCom();
	private: HRESULT AddFilterDirectory(String^ directory);
	private: HRESULT RemoveFilterDirectory(String^ directory);
	private: void closeKernelDriverCom();
	private: void openKernelCommunication();
	private: System::Windows::Forms::Button^  Exit;

	public:
		MyForm(void)
		{
			dragging = false;
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			context.Port = nullptr;
			Globals::Instance->setTextBox(logViewer);
			//context.Completion = nullptr;
			openKernelCommunication();

			trapHandler = new TrapHandler(); // FIXME: protect catch throw
			if (trapHandler == nullptr) {
				throw "aa";
			}
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MyForm()
		{
			CloseHandle(context.Port);
			//CloseHandle(context.Completion);
			if (components)
			{
				delete components;
			}
			delete trapHandler; // FIXME
		}


	protected:

	protected:

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>

		void InitializeComponent(void)
		{
			this->Exit = (gcnew System::Windows::Forms::Button());
			this->InitTraps = (gcnew System::Windows::Forms::Button());
			this->logViewer = (gcnew System::Windows::Forms::TextBox());
			this->AutoKill = (gcnew System::Windows::Forms::CheckBox());
			this->ViewLog = (gcnew System::Windows::Forms::CheckBox());
			this->DisableProtection = (gcnew System::Windows::Forms::CheckBox());
			this->SelectAddRootDir = (gcnew System::Windows::Forms::Button());
			this->SelectRemRootDir = (gcnew System::Windows::Forms::Button());
			this->SuspendLayout();
			// 
			// Exit
			// 
			this->Exit->Location = System::Drawing::Point(475, 59);
			this->Exit->Name = L"Exit";
			this->Exit->Size = System::Drawing::Size(58, 23);
			this->Exit->TabIndex = 8;
			this->Exit->Text = L"Exit";
			this->Exit->UseVisualStyleBackColor = true;
			this->Exit->Click += gcnew System::EventHandler(this, &MyForm::button1_Click);
			// 
			// InitTraps
			// 
			this->InitTraps->Location = System::Drawing::Point(26, 59);
			this->InitTraps->Name = L"InitTraps";
			this->InitTraps->Size = System::Drawing::Size(52, 23);
			this->InitTraps->TabIndex = 9;
			this->InitTraps->Text = L"Init &Traps";
			this->InitTraps->UseVisualStyleBackColor = true;
			this->InitTraps->Click += gcnew System::EventHandler(this, &MyForm::Init_traps);
			// 
			// logViewer
			// 
			this->logViewer->Location = System::Drawing::Point(12, 116);
			this->logViewer->Multiline = true;
			this->logViewer->Name = L"logViewer";
			this->logViewer->ReadOnly = true;
			this->logViewer->Size = System::Drawing::Size(521, 252);
			this->logViewer->TabIndex = 10;
			this->logViewer->Visible = false;
			// 
			// AutoKill
			// 
			this->AutoKill->Appearance = System::Windows::Forms::Appearance::Button;
			this->AutoKill->AutoSize = true;
			this->AutoKill->Checked = true;
			this->AutoKill->CheckState = System::Windows::Forms::CheckState::Checked;
			this->AutoKill->Location = System::Drawing::Point(84, 59);
			this->AutoKill->Name = L"AutoKill";
			this->AutoKill->Size = System::Drawing::Size(52, 23);
			this->AutoKill->TabIndex = 11;
			this->AutoKill->Text = L"AutoKill";
			this->AutoKill->UseVisualStyleBackColor = true;
			this->AutoKill->CheckedChanged += gcnew System::EventHandler(this, &MyForm::autokill_changed);
			// 
			// ViewLog
			// 
			this->ViewLog->Appearance = System::Windows::Forms::Appearance::Button;
			this->ViewLog->AutoSize = true;
			this->ViewLog->Checked = true;
			this->ViewLog->CheckState = System::Windows::Forms::CheckState::Checked;
			this->ViewLog->Location = System::Drawing::Point(142, 59);
			this->ViewLog->Name = L"ViewLog";
			this->ViewLog->Size = System::Drawing::Size(58, 23);
			this->ViewLog->TabIndex = 12;
			this->ViewLog->Text = L"ViewLog";
			this->ViewLog->UseVisualStyleBackColor = true;
			this->ViewLog->CheckedChanged += gcnew System::EventHandler(this, &MyForm::viewlog_changed);
			// 
			// DisableProtection
			// 
			this->DisableProtection->Appearance = System::Windows::Forms::Appearance::Button;
			this->DisableProtection->AutoSize = true;
			this->DisableProtection->Location = System::Drawing::Point(206, 59);
			this->DisableProtection->Name = L"DisableProtection";
			this->DisableProtection->Size = System::Drawing::Size(52, 23);
			this->DisableProtection->TabIndex = 13;
			this->DisableProtection->Text = L"Disable";
			this->DisableProtection->UseVisualStyleBackColor = true;
			this->DisableProtection->CheckedChanged += gcnew System::EventHandler(this, &MyForm::DisableProtection_CheckedChanged);
			// 
			// SelectAddRootDir
			// 
			this->SelectAddRootDir->Location = System::Drawing::Point(265, 59);
			this->SelectAddRootDir->Name = L"SelectAddRootDir";
			this->SelectAddRootDir->Size = System::Drawing::Size(75, 23);
			this->SelectAddRootDir->TabIndex = 14;
			this->SelectAddRootDir->Text = L"AddRootDir";
			this->SelectAddRootDir->UseVisualStyleBackColor = true;
			this->SelectAddRootDir->Click += gcnew System::EventHandler(this, &MyForm::SelectAddRootDir_Click);
			// 
			// SelectRemRootDir
			// 
			this->SelectRemRootDir->Location = System::Drawing::Point(347, 59);
			this->SelectRemRootDir->Name = L"SelectRemRootDir";
			this->SelectRemRootDir->Size = System::Drawing::Size(75, 23);
			this->SelectRemRootDir->TabIndex = 15;
			this->SelectRemRootDir->Text = L"RemRootDir";
			this->SelectRemRootDir->UseVisualStyleBackColor = true;
			this->SelectRemRootDir->Click += gcnew System::EventHandler(this, &MyForm::SelectRemRootDir_Click);
			// 
			// MyForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(545, 423);
			this->Controls->Add(this->SelectRemRootDir);
			this->Controls->Add(this->SelectAddRootDir);
			this->Controls->Add(this->DisableProtection);
			this->Controls->Add(this->ViewLog);
			this->Controls->Add(this->AutoKill);
			this->Controls->Add(this->logViewer);
			this->Controls->Add(this->InitTraps);
			this->Controls->Add(this->Exit);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
			this->Name = L"MyForm";
			this->Text = L"MyForm";
			this->Load += gcnew System::EventHandler(this, &MyForm::MyForm_Load);
			this->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::MyForm_MouseDown);
			this->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::MyForm_MouseMove);
			this->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::MyForm_MouseUp);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void MyForm_Load(System::Object^  sender, System::EventArgs^  e) {
		//Make sure it isn't moving when we open the form.
		this->dragging = false;
	}
	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
		Close();
	}

	private: System::Void MyForm_MouseDown(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
		//tell the form its gonna be draggin'
		this->dragging = true;
		this->offset = Point(e->X, e->Y);
	}

	private: System::Void MyForm_MouseMove(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
		if (this->dragging) {
			Point currentScreenPos = PointToScreen(e->Location);
			Location = Point(currentScreenPos.X - this->offset.X,
				currentScreenPos.Y - this->offset.Y);
		}
	}
	private: System::Void MyForm_MouseUp(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
		this->dragging = false;
	}

	private: System::Void autokill_changed(System::Object^  sender, System::EventArgs^  e) {
		if (AutoKill->Checked) {
			Globals::Instance->setKillStat(TRUE);
		}
		else {
			Globals::Instance->setKillStat(FALSE);
		}
		
	}

private: System::Void viewlog_changed(System::Object^  sender, System::EventArgs^  e) {
	if (ViewLog->Checked) {
		logViewer->Visible = true;
	}
	else {
		logViewer->Visible = false;
	}
}


private: System::Void Init_traps(System::Object^  sender, System::EventArgs^  e) {
	DBOUT("test init traps" << std::endl);
	std::vector<std::wstring> newVec;
	newVec.push_back(L"F:\\test2");
	try {
		trapHandler->initTraps(newVec);
	}
	catch (...) {
		DBOUT("Failed to run traps" << std::endl);
	}
} 
private: System::Void DisableProtection_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
	if (DisableProtection->Checked) {
		Globals::Instance->setMonitorStat(TRUE);
	}
	else {
		Globals::Instance->setMonitorStat(FALSE);
	}
}
private: System::Void SelectRemRootDir_Click(System::Object^  sender, System::EventArgs^  e) {
	String^ selectedDir = nullptr;
	FolderBrowserDialog^ browseDiag = gcnew FolderBrowserDialog;

	//browseDiag->RootFolder = Environment::SpecialFolder::Personal;
	browseDiag->ShowNewFolderButton = false;
	
	if (browseDiag->ShowDialog() == System::Windows::Forms::DialogResult::OK)
	{
		if ((selectedDir = browseDiag->SelectedPath) != nullptr)
		{
			pin_ptr<const wchar_t> wch = PtrToStringChars(selectedDir);
			std::wstring path(wch);
			DBOUT(path << std::endl);
			String^ logHead = gcnew String("<I> Trying to remove directory: ");
			String^ logMsg = String::Concat(logHead, selectedDir);
			logViewer->AppendText(logMsg);

			// do remove dir message
			NTSTATUS hr = RemoveFilterDirectory(selectedDir);
			if (hr == S_OK) {
				String^ logSuccess = gcnew String("<E> Removed filter directory: ");
				String^ logMsgSuc = String::Concat(logSuccess, selectedDir);
				logViewer->AppendText(logMsgSuc);
			}
			else {
				String^ logFail = gcnew String("<E> Failed to remove directory: ");
				String^ logMsgFail = String::Concat(logFail, selectedDir);
				logViewer->AppendText(logMsgFail);
			}

		}
	}
}
private: System::Void SelectAddRootDir_Click(System::Object^  sender, System::EventArgs^  e) {
	String^ selectedDir = nullptr;
	FolderBrowserDialog^ browseDiag = gcnew FolderBrowserDialog;

	//browseDiag->RootFolder = Environment::SpecialFolder::Personal;
	browseDiag->ShowNewFolderButton = false;

	if (browseDiag->ShowDialog() == System::Windows::Forms::DialogResult::OK)
	{
		if ((selectedDir = browseDiag->SelectedPath) != nullptr)
		{
			pin_ptr<const wchar_t> wch = PtrToStringChars(selectedDir);
			std::wstring path(wch);
			DBOUT(path << std::endl);

			String^ logHead = gcnew String("<I> Trying to add directory: ");
			String^ logMsg = String::Concat(logHead, selectedDir);
			logViewer->AppendText(logMsg);

			// do remove dir message
			NTSTATUS hr = AddFilterDirectory(selectedDir);
			if (hr == S_OK) {
				String^ logSuccess = gcnew String("<E> Added filter directory: ");
				String^ logMsgSuc = String::Concat(logSuccess, selectedDir);
				
				logViewer->AppendText(logMsgSuc);
			}
			else {
				String^ logFail = gcnew String("<E> Failed to add directory: ");
				String^ logMsgFail = String::Concat(logFail, selectedDir);
				logViewer->AppendText(logMsgFail);
			}
		}
	}
}

};
}
