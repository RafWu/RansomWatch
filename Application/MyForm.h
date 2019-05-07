#pragma once

#include <Windows.h>
#include <FltUser.h>
#include <comdef.h>
#include "ukshared.h"
#include <iostream>
#include "Common.h"
#include "TrapHandler.h"

#pragma comment(lib, "fltMgr")
#pragma comment(lib, "fltLib")
#pragma comment(lib, "User32")
#pragma comment(lib, "kernel32")

#define NUM_THREADS 2
#define SCAN_PORT L"\\FsFilter"

DWORD ScannerWorker(_In_ PSCANNER_THREAD_CONTEXT Context);

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
	private: HANDLE port;
	private: HANDLE completion;
	private: BOOLEAN kernelComOpen;
	private: void initListenThreads();
	private: BOOLEAN openKernelDriverCom();
	private: void closeKernelDriverCom();
	private: void openKernelCommunication();
	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::Label^  label3;
	private: System::Windows::Forms::Label^  label4;
	private: System::Windows::Forms::Label^  label5;
	private: System::Windows::Forms::Label^  label6;
	private: System::Windows::Forms::Label^  label7;
	private: System::Windows::Forms::Button^  Generate;
	private: System::Windows::Forms::Button^  InitTraps;

	private: System::Windows::Forms::TextBox^  textBox1;

	public:
		MyForm(void)
		{
			dragging = false;
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			port = nullptr;
			completion = nullptr;
			openKernelCommunication();


		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MyForm()
		{
			CloseHandle(port);
			CloseHandle(completion);
			if (components)
			{
				delete components;
			}
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
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->label6 = (gcnew System::Windows::Forms::Label());
			this->label7 = (gcnew System::Windows::Forms::Label());
			this->Generate = (gcnew System::Windows::Forms::Button());
			this->InitTraps = (gcnew System::Windows::Forms::Button());
			this->textBox1 = (gcnew System::Windows::Forms::TextBox());
			this->SuspendLayout();
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(86, 74);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(35, 13);
			this->label1->TabIndex = 1;
			this->label1->Text = L"label1";
			this->label1->Click += gcnew System::EventHandler(this, &MyForm::label1_Click);
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(170, 74);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(35, 13);
			this->label2->TabIndex = 2;
			this->label2->Text = L"label2";
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(254, 74);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(35, 13);
			this->label3->TabIndex = 3;
			this->label3->Text = L"label3";
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(393, 74);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(35, 13);
			this->label4->TabIndex = 4;
			this->label4->Text = L"label4";
			// 
			// label5
			// 
			this->label5->AutoSize = true;
			this->label5->Location = System::Drawing::Point(330, 74);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(35, 13);
			this->label5->TabIndex = 5;
			this->label5->Text = L"label5";
			// 
			// label6
			// 
			this->label6->AutoSize = true;
			this->label6->Location = System::Drawing::Point(467, 74);
			this->label6->Name = L"label6";
			this->label6->Size = System::Drawing::Size(35, 13);
			this->label6->TabIndex = 6;
			this->label6->Text = L"label6";
			// 
			// label7
			// 
			this->label7->AutoSize = true;
			this->label7->Location = System::Drawing::Point(527, 74);
			this->label7->Name = L"label7";
			this->label7->Size = System::Drawing::Size(35, 13);
			this->label7->TabIndex = 7;
			this->label7->Text = L"label7";
			// 
			// Generate
			// 
			this->Generate->Location = System::Drawing::Point(290, 231);
			this->Generate->Name = L"Generate";
			this->Generate->Size = System::Drawing::Size(75, 23);
			this->Generate->TabIndex = 8;
			this->Generate->Text = L"Generate";
			this->Generate->UseVisualStyleBackColor = true;
			this->Generate->Click += gcnew System::EventHandler(this, &MyForm::button1_Click);
			// 
			// InitTraps
			// 
			this->InitTraps->Location = System::Drawing::Point(470, 231);
			this->InitTraps->Name = L"InitTraps";
			this->InitTraps->Size = System::Drawing::Size(75, 23);
			this->InitTraps->TabIndex = 9;
			this->InitTraps->Text = L"Init &Traps";
			this->InitTraps->UseVisualStyleBackColor = true;
			this->InitTraps->Click += gcnew System::EventHandler(this, &MyForm::button2_Click);
			// 
			// textBox1
			// 
			this->textBox1->Location = System::Drawing::Point(13, 119);
			this->textBox1->Multiline = true;
			this->textBox1->Name = L"textBox1";
			this->textBox1->Size = System::Drawing::Size(175, 252);
			this->textBox1->TabIndex = 10;
			// 
			// MyForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(771, 468);
			this->Controls->Add(this->textBox1);
			this->Controls->Add(this->InitTraps);
			this->Controls->Add(this->Generate);
			this->Controls->Add(this->label7);
			this->Controls->Add(this->label6);
			this->Controls->Add(this->label5);
			this->Controls->Add(this->label4);
			this->Controls->Add(this->label3);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->label1);
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
	private: System::Void label1_Click(System::Object^  sender, System::EventArgs^  e) {
	}
	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
		Close();
	}
	private: System::Void button2_Click(System::Object^  sender, System::EventArgs^  e) {
		DBOUT("test init traps" << std::endl);
		std::vector<std::wstring> newVec;
		newVec.push_back(L"F:\\test2");
		try {
			TrapHandler::initTraps(newVec);
		}
		catch (...) {
			DBOUT("Failed to run traps" << std::endl);
		}
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
};
}
