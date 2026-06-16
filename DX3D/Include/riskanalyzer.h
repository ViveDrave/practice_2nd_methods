#pragma once
#include <vector>
#include <string>
#include <numeric>
#include <cmath>
#include <algorithm>

struct RiskResults {
    std::vector<double> ahp_weights;
    std::vector<double> rank_weights;
    std::vector<double> simple_weights;
    std::vector<double> integrated_scores;
    std::vector<int> ranked_indices;
    double kendall_w = 0.0;
};

class RiskAnalyzer {
public:
    std::vector<std::vector<double>> ahp_matrix;
    std::vector<std::vector<double>> expert_scores;
    std::vector<std::vector<int>> expert_ranks;
    std::vector<std::string> threat_names;

    RiskAnalyzer();

    RiskResults calculateAll();

    void setThreatNames(const std::vector<std::string>& names);
    void setExpertScores(const std::vector<std::vector<double>>& scores);
    void setExpertRanks(const std::vector<std::vector<int>>& ranks);
    void setAHPMatrix(const std::vector<std::vector<double>>& matrix);

    std::vector<std::vector<double>> getNormalizedExpertScores() const;
    std::vector<double> getAverageScores() const;

private:
    std::vector<double> calculateAHP();
    std::vector<double> calculateTransformedRanks();
    std::vector<double> calculateWeightAnalysis();
    double calculateKendallW();
    std::vector<double> computeIntegratedScore();
    std::vector<int> getRankedIndices(const std::vector<double>& scores);
    void computeRanksFromScores();

    int num_threats = 6;
    int num_experts = 4;
};
