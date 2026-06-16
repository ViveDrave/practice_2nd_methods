#include "../Include/applicationclass.h"
#include <fstream>
#include <sstream>
#include <ctime>


ApplicationClass::ApplicationClass()
{
	m_Direct3D = 0;
}

ApplicationClass::~ApplicationClass()
{
}

bool ApplicationClass::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
	bool result;
	
	m_Direct3D = new D3DClass;

	result = m_Direct3D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
	if (!result) {
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImFontConfig font_config;
	font_config.OversampleH = 2;
	font_config.OversampleV = 2;

	ImFont* font = io.Fonts->AddFontFromFileTTF(
		"C:\\Windows\\Fonts\\arial.ttf",
		14.0f,
		&font_config,
		io.Fonts->GetGlyphRangesCyrillic()
	);

	ImGui::StyleColorsDark();
	auto& style = ImGui::GetStyle();
	style.FrameRounding = 4.0f;
	style.WindowRounding = 6.0f;
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.16f, 0.35f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.15f, 0.30f, 0.55f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.40f, 0.70f, 1.0f);

	ImPlotStyle& pstyle = ImPlot::GetStyle();
	pstyle.PlotDefaultSize = ImVec2(-1, 350);

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(m_Direct3D->GetDevice(), m_Direct3D->GetDeviceContext());

	return true;
}

void ApplicationClass::Shutdown()
{
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	if (m_Direct3D)
	{
		m_Direct3D->Shutdown();
		delete m_Direct3D;
		m_Direct3D = 0;
	}

	return;
}

bool ApplicationClass::Frame()
{
	bool result;

	result = Render();
	if (!result)
	{
		return false;
	}

	return true;
}

static void RenderAHPTable(RiskAnalyzer& analyzer, bool& needs_recalc)
{
	ImGui::SeparatorText("Матриця попарних порівнянь (шкала Сааті 1-9)");

	if (ImGui::BeginTable("AHP_Matrix", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		ImGui::TableSetupColumn("Загроза");
		for (int i = 0; i < 6; i++)
			ImGui::TableSetupColumn(analyzer.threat_names[i].c_str());
		ImGui::TableHeadersRow();

		for (int row = 0; row < 6; row++)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%s", analyzer.threat_names[row].c_str());

			for (int col = 0; col < 6; col++)
			{
				ImGui::TableSetColumnIndex(col + 1);

				ImGui::PushID(row * 6 + col);
				if (row < col)
				{
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.25f, 0.45f, 0.6f));
					if (ImGui::InputDouble("##v", &analyzer.ahp_matrix[row][col], 0.0, 0.0, "%.2f"))
					{
						analyzer.ahp_matrix[col][row] = 1.0 / analyzer.ahp_matrix[row][col];
						needs_recalc = true;
					}
					ImGui::PopStyleColor();
				}
				else if (row == col)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
					ImGui::Text("1.00");
					ImGui::PopStyleColor();
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
					ImGui::Text("%.2f", analyzer.ahp_matrix[row][col]);
					ImGui::PopStyleColor();
				}
				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}
}

static void RenderExpertScoresTable(RiskAnalyzer& analyzer, bool& needs_recalc)
{
	ImGui::SeparatorText("Експертні оцінки загроз (1-10 балів)");

	if (ImGui::BeginTable("Scores_Table", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		ImGui::TableSetupColumn("Експерт");
		for (int i = 0; i < 6; i++)
			ImGui::TableSetupColumn(analyzer.threat_names[i].c_str());
		ImGui::TableHeadersRow();

		for (int exp = 0; exp < 4; exp++)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Експерт %d", exp + 1);

			for (int thr = 0; thr < 6; thr++)
			{
				ImGui::TableSetColumnIndex(thr + 1);
				ImGui::PushID(exp * 6 + thr + 100);

				double val = analyzer.expert_scores[exp][thr];
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::InputDouble("##s", &val, 0.0, 0.0, "%.0f"))
				{
					if (val < 1.0) val = 1.0;
					if (val > 10.0) val = 10.0;
					analyzer.expert_scores[exp][thr] = val;
					needs_recalc = true;
				}
				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}
}

static void RenderExpertRanksTable(RiskAnalyzer& analyzer, bool& needs_recalc)
{
	ImGui::SeparatorText("Трансформованi ранги (1 - найвищий, 6 - найнижчий)");
	ImGui::TextDisabled("Ранги оновлюються автоматично при змiнi оцiнок. Для ручного введення змiнiть значення.");

	if (ImGui::BeginTable("Ranks_Table", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		ImGui::TableSetupColumn("Експерт");
		for (int i = 0; i < 6; i++)
			ImGui::TableSetupColumn(analyzer.threat_names[i].c_str());
		ImGui::TableHeadersRow();

		for (int exp = 0; exp < 4; exp++)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Експерт %d", exp + 1);

			for (int thr = 0; thr < 6; thr++)
			{
				ImGui::TableSetColumnIndex(thr + 1);
				ImGui::PushID(exp * 6 + thr + 200);

				int rank = analyzer.expert_ranks[exp][thr];
				ImGui::SetNextItemWidth(-FLT_MIN);

				bool is_dup = false;
				for (int t = 0; t < 6; t++)
				{
					if (t != thr && analyzer.expert_ranks[exp][t] == rank)
						is_dup = true;
				}
				if (is_dup || rank < 1 || rank > 6)
					ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.6f, 0.1f, 0.1f, 0.6f));

				if (ImGui::InputInt("##r", &analyzer.expert_ranks[exp][thr], 0))
				{
					if (analyzer.expert_ranks[exp][thr] < 1) analyzer.expert_ranks[exp][thr] = 1;
					if (analyzer.expert_ranks[exp][thr] > 6) analyzer.expert_ranks[exp][thr] = 6;
					needs_recalc = true;
				}

				if (is_dup || rank < 1 || rank > 6)
					ImGui::PopStyleColor();

				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}
}

static void RenderResultsTable(const RiskAnalyzer& analyzer, const RiskResults& results)
{
	ImGui::SeparatorText("Ранжування загроз за iнтегрованою оцiнкою ризику");

	if (ImGui::BeginTable("Results_Table", 7,
		ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable))
	{
		ImGui::TableSetupColumn("Ранг");
		ImGui::TableSetupColumn("Загроза");
		ImGui::TableSetupColumn("AHP");
		ImGui::TableSetupColumn("Трансф. ранги");
		ImGui::TableSetupColumn("Ваговий ан.");
		ImGui::TableSetupColumn("Iнтегрована");
		ImGui::TableSetupColumn("Рiвень ризику");
		ImGui::TableHeadersRow();

		for (int idx = 0; idx < 6; idx++)
		{
			int ti = results.ranked_indices[idx];

			ImGui::TableNextRow();

			double risk = results.integrated_scores[ti];
			ImVec4 risk_color;
			const char* risk_label;
			if (risk >= 0.20) {
				risk_color = ImVec4(0.95f, 0.20f, 0.20f, 1.0f);
				risk_label = "Критичний";
			}
			else if (risk >= 0.15) {
				risk_color = ImVec4(0.95f, 0.55f, 0.10f, 1.0f);
				risk_label = "Високий";
			}
			else if (risk >= 0.10) {
				risk_color = ImVec4(0.90f, 0.85f, 0.10f, 1.0f);
				risk_label = "Середнiй";
			}
			else {
				risk_color = ImVec4(0.20f, 0.75f, 0.20f, 1.0f);
				risk_label = "Низький";
			}

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%d", idx + 1);

			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%s", analyzer.threat_names[ti].c_str());

			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%.4f", results.ahp_weights[ti]);

			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%.4f", results.rank_weights[ti]);

			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%.4f", results.simple_weights[ti]);

			ImGui::TableSetColumnIndex(5);
			ImGui::PushStyleColor(ImGuiCol_Text, risk_color);
			ImGui::Text("%.4f", risk);
			ImGui::PopStyleColor();

			ImGui::TableSetColumnIndex(6);
			ImGui::PushStyleColor(ImGuiCol_Text, risk_color);
			ImGui::TextUnformatted(risk_label);
			ImGui::PopStyleColor();
		}
		ImGui::EndTable();
	}
}

static void RenderNormalizedTable(const RiskAnalyzer& analyzer)
{
	ImGui::SeparatorText("Нормалiзованi експертнi оцiнки");
	auto normalized = analyzer.getNormalizedExpertScores();
	auto averages = analyzer.getAverageScores();

	if (ImGui::BeginTable("Norm_Table", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		ImGui::TableSetupColumn("Загроза");
		for (int e = 0; e < 4; e++)
		{
			char label[16];
			sprintf_s(label, "Експ %d", e + 1);
			ImGui::TableSetupColumn(label);
		}
		ImGui::TableSetupColumn("Середнє");
		ImGui::TableSetupColumn("Нормоване");
		ImGui::TableHeadersRow();

		for (int j = 0; j < 6; j++)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%s", analyzer.threat_names[j].c_str());

			double norm_avg = 0.0;
			for (int e = 0; e < 4; e++)
			{
				ImGui::TableSetColumnIndex(e + 1);
				ImGui::Text("%.3f", normalized[e][j]);
				norm_avg += normalized[e][j];
			}
			norm_avg /= 4.0;

			ImGui::TableSetColumnIndex(5);
			ImGui::Text("%.1f", averages[j]);

			ImGui::TableSetColumnIndex(6);
			ImGui::Text("%.4f", norm_avg);
		}
		ImGui::EndTable();
	}
}

static void RenderKendallW(const RiskResults& results)
{
	double w = results.kendall_w;
	ImGui::SeparatorText("Коефiцiєнт конкордацiї Кендалла");

	ImGui::Text("W = %.4f", w);

	const char* interpretation;
	ImVec4 color;
	if (w >= 0.7) {
		interpretation = "Висока узгодженiсть експертiв";
		color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
	}
	else if (w >= 0.3) {
		interpretation = "Середня узгодженiсть експертiв";
		color = ImVec4(0.9f, 0.85f, 0.1f, 1.0f);
	}
	else {
		interpretation = "Низька узгодженiсть експертiв";
		color = ImVec4(0.95f, 0.2f, 0.2f, 1.0f);
	}

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::TextUnformatted(interpretation);
	ImGui::PopStyleColor();

	ImGui::Spacing();
	ImGui::TextDisabled("Шкала: W < 0.3 - низька; 0.3-0.7 - середня; W > 0.7 - висока");
}

void ApplicationClass::SaveReportCSV(const char* filename)
{
	std::ofstream file(filename);
	if (!file) return;

	auto now = std::time(nullptr);
	char time_buf[64];
	ctime_s(time_buf, sizeof(time_buf), &now);
	time_buf[strcspn(time_buf, "\n")] = 0;

	m_results = m_analyzer.calculateAll();

	file << "ЗВIТ З ОЦIНЮВАННЯ КIБЕРРИЗИКIВ\n";
	file << "Дата: " << time_buf << "\n\n";

	file << "ВХIДНI ДАНI: ЕКСПЕРТНI ОЦIНКИ\n";
	file << "Загроза";
	for (int e = 0; e < 4; e++) file << ",Експерт " << (e + 1);
	file << "\n";
	for (int j = 0; j < 6; j++) {
		file << m_analyzer.threat_names[j];
		for (int e = 0; e < 4; e++)
			file << "," << m_analyzer.expert_scores[e][j];
		file << "\n";
	}

	file << "\nЕКСПЕРТНI РАНГИ\n";
	file << "Загроза";
	for (int e = 0; e < 4; e++) file << ",Експерт " << (e + 1);
	file << "\n";
	for (int j = 0; j < 6; j++) {
		file << m_analyzer.threat_names[j];
		for (int e = 0; e < 4; e++)
			file << "," << m_analyzer.expert_ranks[e][j];
		file << "\n";
	}

	file << "\nРЕЗУЛЬТАТИ РОЗРАХУНКIВ\n";
	file << "Коефiцiєнт конкордацiї Кендалла: " << m_results.kendall_w << "\n\n";

	file << "Ранг,Загроза,AHP,Трансф.ранги,Ваговий ан.,Iнтегрована,Рiвень ризику\n";
	for (int idx = 0; idx < 6; idx++)
	{
		int ti = m_results.ranked_indices[idx];
		double r = m_results.integrated_scores[ti];
		const char* level = "Низький";
		if (r >= 0.20) level = "Критичний";
		else if (r >= 0.15) level = "Високий";
		else if (r >= 0.10) level = "Середнiй";

		file << (idx + 1) << ","
			<< m_analyzer.threat_names[ti] << ","
			<< m_results.ahp_weights[ti] << ","
			<< m_results.rank_weights[ti] << ","
			<< m_results.simple_weights[ti] << ","
			<< r << ","
			<< level << "\n";
	}

	file << "\nНОРМАЛIЗОВАНI ОЦIНКИ\n";
	auto norm = m_analyzer.getNormalizedExpertScores();
	file << "Загроза";
	for (int e = 0; e < 4; e++) file << ",Експерт " << (e + 1);
	file << "\n";
	for (int j = 0; j < 6; j++) {
		file << m_analyzer.threat_names[j];
		for (int e = 0; e < 4; e++)
			file << "," << norm[e][j];
		file << "\n";
	}

	file.close();
}

void ApplicationClass::imgui_render()
{
	if (m_needsRecalc) {
		m_results = m_analyzer.calculateAll();
		m_needsRecalc = false;
	}

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::Begin("CyberRiskAnalyzer", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_MenuBar);
	ImGui::PopStyleVar();

	if (ImGui::BeginMenuBar())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.7f, 1.0f, 1.0f));
		ImGui::TextUnformatted("ОЦIНЮВАННЯ КIБЕРРИЗИКIВ");
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(20, 0));
		ImGui::TextDisabled("|");

		if (ImGui::MenuItem("  Зберегти звiт (CSV)  "))
		{
			OPENFILENAMEA ofn = {};
			char szFile[260] = { 0 };
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = NULL;
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "CSV Files\0*.csv\0All Files\0*.*\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrDefExt = "csv";
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
			if (GetSaveFileNameA(&ofn))
			{
				SaveReportCSV(ofn.lpstrFile);
			}
		}

		ImGui::EndMenuBar();
	}

	if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_FittingPolicyScroll))
	{
		if (ImGui::BeginTabItem("Вхiднi данi"))
		{
			ImGui::Spacing();
			RenderAHPTable(m_analyzer, m_needsRecalc);
			ImGui::Spacing();
			RenderExpertScoresTable(m_analyzer, m_needsRecalc);
			ImGui::Spacing();
			RenderExpertRanksTable(m_analyzer, m_needsRecalc);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Результати"))
		{
			ImGui::Spacing();
			RenderKendallW(m_results);
			ImGui::Spacing();
			RenderResultsTable(m_analyzer, m_results);
			ImGui::Spacing();
			RenderNormalizedTable(m_analyzer);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Графiки"))
		{
			ImGui::Spacing();

			ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0.08f, 0.08f, 0.12f, 1.0f));

			if (ImPlot::BeginPlot("Порiвняння методiв оцiнювання", ImVec2(-1, 350)))
			{
				ImPlot::SetupAxes("Загрози", "Ваговий коефiцiєнт",
					ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

				double positions[] = { 0, 1, 2, 3, 4, 5 };
				const char* labels[] = {
					m_analyzer.threat_names[0].c_str(),
					m_analyzer.threat_names[1].c_str(),
					m_analyzer.threat_names[2].c_str(),
					m_analyzer.threat_names[3].c_str(),
					m_analyzer.threat_names[4].c_str(),
					m_analyzer.threat_names[5].c_str()
				};
				ImPlot::SetupAxisTicks(ImAxis_X1, positions, 6, labels);

				double xs_ahp[6], xs_rank[6], xs_weight[6];
				for (int i = 0; i < 6; i++) {
					xs_ahp[i] = i - 0.30;
					xs_rank[i] = i;
					xs_weight[i] = i + 0.30;
				}

				ImPlot::PlotBars<double>("Метод Саатi (AHP)",
					xs_ahp, m_results.ahp_weights.data(), 6, 0.25,
					ImPlotSpec(ImPlotProp_FillColor, IM_COL32(50, 120, 220, 200)));

				ImPlot::PlotBars<double>("Трансформованi ранги",
					xs_rank, m_results.rank_weights.data(), 6, 0.25,
					ImPlotSpec(ImPlotProp_FillColor, IM_COL32(50, 180, 80, 200)));

				ImPlot::PlotBars<double>("Ваговий аналiз",
					xs_weight, m_results.simple_weights.data(), 6, 0.25,
					ImPlotSpec(ImPlotProp_FillColor, IM_COL32(220, 150, 30, 200)));

				ImPlot::EndPlot();
			}

			ImGui::Spacing();

			if (ImPlot::BeginPlot("Розподiл iнтегрованого ризику", ImVec2(-1, 400)))
			{
				ImPlot::SetupAxes(nullptr, nullptr,
					ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);

				const char* pie_labels[6];
				for (int i = 0; i < 6; i++)
					pie_labels[i] = m_analyzer.threat_names[i].c_str();

				double integrated[6];
				for (int i = 0; i < 6; i++)
					integrated[i] = m_results.integrated_scores[i];

				ImPlot::PlotPieChart(pie_labels, integrated, 6, 0, 0, 0.85,
					"%.2f", 90,
					ImPlotSpec(ImPlotProp_Flags, ImPlotPieChartFlags_Exploding));

				ImPlot::EndPlot();
			}

			ImPlot::PopStyleColor();

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

ID3D11Device* ApplicationClass::GetDevice()
{
	return m_Direct3D->GetDevice();
}

ID3D11DeviceContext* ApplicationClass::GetDeviceContext()
{
	return m_Direct3D->GetDeviceContext();
}

bool ApplicationClass::Render()
{
	m_Direct3D->BeginScene(0.025f, 0.025f, 0.035f, 1.0f);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	imgui_render();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	m_Direct3D->EndScene();

	return true;
}
