#pragma once

#include <comdef.h>
#include <iostream>
#include "Common.h"
#include "TrapHandler.h"
#include "FilterWorker.h"
#include "BackupService.h"

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
	private: System::Windows::Forms::PictureBox^ pictureBox1;
	private: System::Windows::Forms::CheckBox^ Verbose;
	private: System::Windows::Forms::Label^ AppNameTitle;
	private: System::Windows::Forms::TextBox^  logViewer;

	private: HRESULT initWorkThread();
	private: BOOLEAN openKernelDriverCom();
	private: HRESULT AddFilterDirectory(String^ directory);
	private: HRESULT RemoveFilterDirectory(String^ directory);
	private: void closeKernelDriverCom();
	private: BOOLEAN openKernelCommunication();
	private: System::Windows::Forms::Button^  Exit;
	private: System::Windows::Forms::Button^ MinimizeButton;
	public: BackupService^ service;
	public:
		MyForm(void)
		{
			dragging = false;
			InitializeComponent();
			Reflection::Assembly^ pxAssembly = Reflection::Assembly::GetExecutingAssembly();
			String^ pxResName = pxAssembly->GetName()->Name + ".Resource";
			Resources::ResourceManager^ s_pxResourceManager = (gcnew Resources::ResourceManager(pxResName, pxAssembly));
			this->pictureBox1->Image = (cli::safe_cast<Drawing::Bitmap^>(s_pxResourceManager->GetObject("icon")));
			this->Icon = (cli::safe_cast<System::Drawing::Icon^>(s_pxResourceManager->GetObject("iconSmall")));
			this->Exit->Image = (cli::safe_cast<Drawing::Bitmap^>(s_pxResourceManager->GetObject("close")));
			this->MinimizeButton->Image = (cli::safe_cast<Drawing::Bitmap^>(s_pxResourceManager->GetObject("min")));
			context.Port = nullptr;
			Globals::Instance->setTextBox(logViewer);
			
			if (!openKernelCommunication()) { // fail to open communication
				String^ strMessage = String::Concat("<E> Failed to open communication with driver", System::Environment::NewLine);
				String^ caption = "Driver connection error";
				//Globals::Instance->postLogMessage(strMessage, PRIORITY_PRINT);
				System::Windows::Forms::MessageBoxButtons buttons = System::Windows::Forms::MessageBoxButtons::OK;
				System::Windows::Forms::MessageBox::Show(strMessage, caption, buttons);
				//throw "ApplicationFailStart";
			}

			try {
				service = gcnew BackupService();
				Globals::Instance->setBackupService(service);
				trapHandler = new TrapHandler();
			}
			catch (Exception ^ e) {
				String^ strMessage = String::Concat("<E> Failed to start backup service or traps handler: ", e->Message, System::Environment::NewLine);
				String^ caption = "Application internal error";
				Globals::Instance->postLogMessage(strMessage, PRIORITY_PRINT);
				System::Windows::Forms::MessageBoxButtons buttons = System::Windows::Forms::MessageBoxButtons::OK;
				System::Windows::Forms::MessageBox::Show(strMessage, caption, buttons);
				//throw "ApplicationFailStart";
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
			System::ComponentModel::ComponentResourceManager^ resources = (gcnew System::ComponentModel::ComponentResourceManager(MyForm::typeid));
			this->Exit = (gcnew System::Windows::Forms::Button());
			this->MinimizeButton = (gcnew System::Windows::Forms::Button());
			this->logViewer = (gcnew System::Windows::Forms::TextBox());
			this->AutoKill = (gcnew System::Windows::Forms::CheckBox());
			this->ViewLog = (gcnew System::Windows::Forms::CheckBox());
			this->DisableProtection = (gcnew System::Windows::Forms::CheckBox());
			this->SelectAddRootDir = (gcnew System::Windows::Forms::Button());
			this->SelectRemRootDir = (gcnew System::Windows::Forms::Button());
			this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
			this->Verbose = (gcnew System::Windows::Forms::CheckBox());
			this->AppNameTitle = (gcnew System::Windows::Forms::Label());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->BeginInit();
			this->SuspendLayout();
			
			this->Exit->Location = System::Drawing::Point(512, 2);
			this->Exit->Name = L"Exit";
			this->Exit->Size = System::Drawing::Size(22, 22); //23
			this->Exit->TabIndex = 8;
			//this->Exit->Text = L"Exit";
			this->Exit->UseVisualStyleBackColor = true;
			this->Exit->Click += gcnew System::EventHandler(this, &MyForm::button1_Click);
			
			this->MinimizeButton->Location = System::Drawing::Point(486, 2);
			this->MinimizeButton->Name = L"MinimizeButton";
			this->MinimizeButton->Size = System::Drawing::Size(22, 22);
			this->MinimizeButton->TabIndex = 8;
			//this->MinimizeButton->Text = L"Min";
			this->MinimizeButton->UseVisualStyleBackColor = true;
			this->MinimizeButton->Click += gcnew System::EventHandler(this, &MyForm::MinimizeButton_Click);
			
			this->logViewer->Location = System::Drawing::Point(13, 113);
			this->logViewer->Multiline = true;
			this->logViewer->Name = L"logViewer";
			this->logViewer->ReadOnly = true;
			this->logViewer->ScrollBars = System::Windows::Forms::ScrollBars::Vertical;
			this->logViewer->Size = System::Drawing::Size(521, 252);
			this->logViewer->TabIndex = 10;

			this->AutoKill->Appearance = System::Windows::Forms::Appearance::Button;
			this->AutoKill->Checked = true;
			this->AutoKill->CheckState = System::Windows::Forms::CheckState::Checked;
			this->AutoKill->Location = System::Drawing::Point(94, 42);
			this->AutoKill->Name = L"AutoKill";
			this->AutoKill->Size = System::Drawing::Size(120, 23);
			this->AutoKill->TabIndex = 11;
			this->AutoKill->Text = L"Kill Processes";
			this->AutoKill->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			this->AutoKill->UseVisualStyleBackColor = true;
			this->AutoKill->CheckedChanged += gcnew System::EventHandler(this, &MyForm::autokill_changed);

			this->ViewLog->Appearance = System::Windows::Forms::Appearance::Button;
			this->ViewLog->Checked = true;
			this->ViewLog->CheckState = System::Windows::Forms::CheckState::Checked;
			this->ViewLog->Location = System::Drawing::Point(237, 71);
			this->ViewLog->Name = L"ViewLog";
			this->ViewLog->Size = System::Drawing::Size(120, 23);
			this->ViewLog->TabIndex = 12;
			this->ViewLog->Text = L"View Log";
			this->ViewLog->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			this->ViewLog->UseVisualStyleBackColor = true;
			this->ViewLog->CheckedChanged += gcnew System::EventHandler(this, &MyForm::viewlog_changed);

			this->DisableProtection->Appearance = System::Windows::Forms::Appearance::Button;
			this->DisableProtection->Location = System::Drawing::Point(237, 42);
			this->DisableProtection->Name = L"DisableProtection";
			this->DisableProtection->Size = System::Drawing::Size(120, 23);
			this->DisableProtection->TabIndex = 13;
			this->DisableProtection->Text = L"Disable Monitor";
			this->DisableProtection->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			this->DisableProtection->UseVisualStyleBackColor = true;
			this->DisableProtection->CheckedChanged += gcnew System::EventHandler(this, &MyForm::DisableProtection_CheckedChanged);

			this->SelectAddRootDir->Location = System::Drawing::Point(380, 71);
			this->SelectAddRootDir->Name = L"SelectAddRootDir";
			this->SelectAddRootDir->Size = System::Drawing::Size(120, 23);
			this->SelectAddRootDir->TabIndex = 14;
			this->SelectAddRootDir->Text = L"Add Directory";
			this->SelectAddRootDir->UseVisualStyleBackColor = true;
			this->SelectAddRootDir->Click += gcnew System::EventHandler(this, &MyForm::SelectAddRootDir_Click);
			this->SelectRemRootDir->Location = System::Drawing::Point(380, 42);
			this->SelectRemRootDir->Name = L"SelectRemRootDir";
			this->SelectRemRootDir->Size = System::Drawing::Size(120, 23);
			this->SelectRemRootDir->TabIndex = 15;
			this->SelectRemRootDir->Text = L"Remove Directory";
			this->SelectRemRootDir->UseVisualStyleBackColor = true;
			this->SelectRemRootDir->Click += gcnew System::EventHandler(this, &MyForm::SelectRemRootDir_Click);

			this->pictureBox1->Location = System::Drawing::Point(13, 13);
			this->pictureBox1->Name = L"pictureBox1";
			this->pictureBox1->Size = System::Drawing::Size(73, 69);
			this->pictureBox1->SizeMode = System::Windows::Forms::PictureBoxSizeMode::Zoom;
			this->pictureBox1->TabIndex = 16;
			this->pictureBox1->TabStop = false;

			this->Verbose->Appearance = System::Windows::Forms::Appearance::Button;
			this->Verbose->Location = System::Drawing::Point(94, 71);
			this->Verbose->Name = L"Verbose";
			this->Verbose->Size = System::Drawing::Size(120, 23);
			this->Verbose->TabIndex = 17;
			this->Verbose->Text = L"Verbose Mode";
			this->Verbose->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			this->Verbose->UseVisualStyleBackColor = true;
			this->Verbose->CheckedChanged += gcnew System::EventHandler(this, &MyForm::Verbose_CheckedChanged);
			
			this->AppNameTitle->Font = (gcnew System::Drawing::Font(L"Guttman Miryam", 15.75, static_cast<System::Drawing::FontStyle>((System::Drawing::FontStyle::Bold | System::Drawing::FontStyle::Italic)),
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(177)));
			this->AppNameTitle->Location = System::Drawing::Point(210, 11);
			this->AppNameTitle->Name = L"AppNameTitle";
			this->AppNameTitle->Size = System::Drawing::Size(170, 23);
			this->AppNameTitle->TabIndex = 18;
			this->AppNameTitle->Text = L"Ransom Watch";
			this->AppNameTitle->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			
			
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->AutoSize = true;
			this->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
			this->ClientSize = System::Drawing::Size(545, 373);
			this->Controls->Add(this->AppNameTitle);
			this->Controls->Add(this->Verbose);
			this->Controls->Add(this->pictureBox1);
			this->Controls->Add(this->SelectRemRootDir);
			this->Controls->Add(this->SelectAddRootDir);
			this->Controls->Add(this->DisableProtection);
			this->Controls->Add(this->ViewLog);
			this->Controls->Add(this->AutoKill);
			this->Controls->Add(this->logViewer);
			this->Controls->Add(this->Exit);
			this->Controls->Add(this->MinimizeButton);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
			this->Name = L"MyForm";
			this->Padding = System::Windows::Forms::Padding(0, 0, 0, 10);
			this->Text = L"Ransom Watch";
			this->Load += gcnew System::EventHandler(this, &MyForm::MyForm_Load);
			this->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::MyForm_MouseDown);
			this->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::MyForm_MouseMove);
			this->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MyForm::MyForm_MouseUp);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->EndInit();
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
			String^ logMsg = String::Concat(logHead, selectedDir, System::Environment::NewLine);
			Globals::Instance->postLogMessage(logMsg, PRIORITY_PRINT);

			// do remove dir message
			NTSTATUS hr = RemoveFilterDirectory(selectedDir);
			if (hr == S_OK) {
				String^ logSuccess = gcnew String("<I> Removed filter directory: ");
				String^ logMsgSuc = String::Concat(logSuccess, selectedDir, System::Environment::NewLine);
				Globals::Instance->postLogMessage(logMsgSuc, PRIORITY_PRINT);
			}
			else {
				String^ logFail = gcnew String("<E> Failed to remove directory: ");
				String^ logMsgFail = String::Concat(logFail, selectedDir, System::Environment::NewLine);
				Globals::Instance->postLogMessage(logMsgFail, PRIORITY_PRINT);
			}

			// handle traps

			trapHandler->remDirTraps(selectedDir);
			
			if (service->RemoveSnapshotDirectory(selectedDir)) {
				Globals::Instance->postLogMessage(String::Concat("<I> Snapshot directory deleted: ", selectedDir, System::Environment::NewLine), PRIORITY_PRINT);
			}
			else {
				Globals::Instance->postLogMessage(String::Concat("<E> Snapshot directory not deleted: ", selectedDir, System::Environment::NewLine), PRIORITY_PRINT);
			}

			if (Globals::Instance->getCommCloseStat() || context.Port == nullptr) { // no comm
				Globals::Instance->postLogMessage(String::Concat("<E> Comm closed", System::Environment::NewLine), PRIORITY_PRINT);
				return;
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
			String^ logMsg = String::Concat(logHead, selectedDir, System::Environment::NewLine);
			Globals::Instance->postLogMessage(logMsg, PRIORITY_PRINT);
			
			if (service->SnapshotDirectory(selectedDir)) {
				Globals::Instance->postLogMessage(String::Concat("<I> Snapshot directory: ", selectedDir, " successfully", System::Environment::NewLine), PRIORITY_PRINT);
			}
			else {
				Globals::Instance->postLogMessage(String::Concat("<E> Snapshot directory: ", selectedDir, " failed", System::Environment::NewLine), PRIORITY_PRINT);
			}

			if (Globals::Instance->getCommCloseStat() || context.Port == nullptr) { // no comm
				Globals::Instance->postLogMessage(String::Concat("<E> Comm closed", System::Environment::NewLine), PRIORITY_PRINT);
				return;
			}
			
			// handle traps

			trapHandler->initDirTraps(selectedDir);

			// do add dir message
			NTSTATUS hr = AddFilterDirectory(selectedDir);
			if (hr == S_OK) {
				String^ logSuccess = gcnew String("<I> Added filter directory: ");
				String^ logMsgSuc = String::Concat(logSuccess, selectedDir, System::Environment::NewLine);
				Globals::Instance->postLogMessage(logMsgSuc, PRIORITY_PRINT);
			}
			else {
				String^ logFail = gcnew String("<E> Failed to add directory: ");
				String^ logMsgFail = String::Concat(logFail, selectedDir, System::Environment::NewLine);
				Globals::Instance->postLogMessage(logMsgFail, PRIORITY_PRINT);
			}
		}
	}
}

private: System::Void Verbose_CheckedChanged(System::Object^ sender, System::EventArgs^ e) {
	if (Verbose->Checked) {
		Globals::Instance->setVerbose(TRUE);
	}
	else {
		Globals::Instance->setVerbose(FALSE);
	}
}

private: System::Void MinimizeButton_Click(System::Object^ sender, System::EventArgs^ e) {
	if (MyForm::WindowState != FormWindowState::Minimized) MyForm::WindowState = FormWindowState::Minimized;

}

};
}
