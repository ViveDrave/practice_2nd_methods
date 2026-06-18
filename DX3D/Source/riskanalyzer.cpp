#include "../Include/riskanalyzer.h"

RiskAnalyzer::RiskAnalyzer() {
    threat_names = {"Фішинг", "Malware", "Витік даних", "Акаунт", "Внутрішня", "DDoS"};

    ahp_matrix = {
        {1.0, 1.0, 1.0, 2.0, 2.0, 3.0},
        {1.0, 1.0, 1.0, 2.0, 2.0, 3.0},
        {1.0, 1.0, 1.0, 1.0, 2.0, 2.0},
        {0.5, 0.5, 1.0, 1.0, 1.0, 2.0},
        {0.5, 0.5, 0.5, 1.0, 1.0, 2.0},
        {0.33, 0.33, 0.5, 0.5, 0.5, 1.0}
    };

    expert_scores = {
        {9, 8, 7, 5, 6, 9},
        {8, 8, 8, 6, 6, 7},
        {8, 7, 7, 6, 5, 8},
        {8, 9, 7, 6, 7, 8}
    };

    expert_ranks = {
        {6, 4, 3, 1, 2, 6},
        {6, 6, 6, 2, 2, 3},
        {6, 4, 4, 2, 1, 6},
        {5, 6, 3, 1, 3, 5}
    };

    computeRanksFromScores();
}

RiskResults RiskAnalyzer::calculateAll() {
    RiskResults res;
    if (!ahp_matrix.empty())     res.ahp_weights = calculateAHP();
    if (!expert_ranks.empty())   res.rank_weights = calculateTransformedRanks();
    if (!expert_scores.empty())  res.simple_weights = calculateWeightAnalysis();
    if (!expert_ranks.empty())   res.kendall_w = calculateKendallW();
    res.integrated_scores = computeIntegratedScore();
    res.ranked_indices = getRankedIndices(res.integrated_scores);
    return res;
}

void RiskAnalyzer::setThreatNames(const std::vector<std::string>& names) {
    if (names.size() == num_threats) {
        threat_names = names;
    }
}

void RiskAnalyzer::setExpertScores(const std::vector<std::vector<double>>& scores) {
    if (scores.size() == num_experts && scores[0].size() == num_threats) {
        expert_scores = scores;
        computeRanksFromScores();
    }
}

void RiskAnalyzer::setExpertRanks(const std::vector<std::vector<int>>& ranks) {
    if (ranks.size() == num_experts && ranks[0].size() == num_threats) {
        expert_ranks = ranks;
    }
}

void RiskAnalyzer::setAHPMatrix(const std::vector<std::vector<double>>& matrix) {
    if (matrix.size() == num_threats && matrix[0].size() == num_threats) {
        ahp_matrix = matrix;
    }
}

std::vector<std::vector<double>> RiskAnalyzer::getNormalizedExpertScores() const {
    std::vector<std::vector<double>> normalized(num_experts, std::vector<double>(num_threats, 0.0));
    for (int i = 0; i < num_experts; ++i) {
        double sum = std::accumulate(expert_scores[i].begin(), expert_scores[i].end(), 0.0);
        if (sum > 0) {
            for (int j = 0; j < num_threats; ++j) {
                normalized[i][j] = expert_scores[i][j] / sum;
            }
        }
    }
    return normalized;
}

std::vector<double> RiskAnalyzer::getAverageScores() const {
    std::vector<double> averages(num_threats, 0.0);
    for (int j = 0; j < num_threats; ++j) {
        double sum = 0.0;
        for (int i = 0; i < num_experts; ++i) {
            sum += expert_scores[i][j];
        }
        averages[j] = sum / num_experts;
    }
    return averages;
}

// --- Private methods ---

std::vector<double> RiskAnalyzer::calculateAHP() {
    int n = num_threats;
    std::vector<double> weights(n, 0.0);
    double sum = 0.0;

    for (int i = 0; i < n; ++i) {
        double product = 1.0;
        for (int j = 0; j < n; ++j) {
            product *= ahp_matrix[i][j];
        }
        weights[i] = std::pow(product, 1.0 / n);
        sum += weights[i];
    }

    for (double& w : weights) w /= sum;
    return weights;
}

std::vector<double> RiskAnalyzer::calculateTransformedRanks() {
    std::vector<double> weights(num_threats, 0.0);
    double total_transformed_sum = 0.0;

    for (int j = 0; j < num_threats; ++j) {
        double threat_sum = 0.0;
        for (int i = 0; i < num_experts; ++i) {
            threat_sum += (num_threats - expert_ranks[i][j] + 1);
        }
        weights[j] = threat_sum;
        total_transformed_sum += threat_sum;
    }

    for (int j = 0; j < num_threats; ++j) {
        weights[j] /= total_transformed_sum;
    }

    return weights;
}

std::vector<double> RiskAnalyzer::calculateWeightAnalysis() {
    std::vector<double> averages(num_threats, 0.0);
    double total_avg_sum = 0.0;

    for (int j = 0; j < num_threats; ++j) {
        double sum = 0.0;
        for (int i = 0; i < num_experts; ++i) {
            sum += expert_scores[i][j];
        }
        averages[j] = sum / num_experts;
        total_avg_sum += averages[j];
    }

    std::vector<double> weights(num_threats, 0.0);
    for (int j = 0; j < num_threats; ++j) {
        weights[j] = averages[j] / total_avg_sum;
    }

    return weights;
}

double RiskAnalyzer::calculateKendallW() {
    int m = num_experts;
    int n = num_threats;

    std::vector<double> rank_sums(n, 0.0);
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < m; ++i) {
            rank_sums[j] += expert_ranks[i][j];
        }
    }

    double mean_sum = 0.5 * m * (n + 1);

    double S = 0.0;
    for (int j = 0; j < n; ++j) {
        S += std::pow(rank_sums[j] - mean_sum, 2);
    }

    double denominator = (m * m) * (std::pow(n, 3) - n);
    if (denominator == 0) return 0.0;

    return (12.0 * S) / denominator;
}

std::vector<double> RiskAnalyzer::computeIntegratedScore() {
    std::vector<double> ahp = calculateAHP();
    std::vector<double> ranks = calculateTransformedRanks();
    std::vector<double> weight = calculateWeightAnalysis();

    std::vector<double> integrated(num_threats, 0.0);
    for (int i = 0; i < num_threats; ++i) {
        integrated[i] = (ahp[i] + ranks[i] + weight[i]) / 3.0;
    }
    return integrated;
}

std::vector<int> RiskAnalyzer::getRankedIndices(const std::vector<double>& scores) {
    std::vector<int> indices(num_threats);
    for (int i = 0; i < num_threats; ++i) indices[i] = i;

    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return scores[a] > scores[b];
    });

    return indices;
}

void RiskAnalyzer::computeRanksFromScores() {
    for (int i = 0; i < num_experts; ++i) {
        std::vector<std::pair<double, int>> scored;
        for (int j = 0; j < num_threats; ++j) {
            scored.emplace_back(expert_scores[i][j], j);
        }

        std::sort(scored.begin(), scored.end(),
            [](const auto& a, const auto& b) { return a.first > b.first; });

        int rank = 1;
        for (int k = 0; k < (int)scored.size();) {
            int tie_end = k;
            while (tie_end < (int)scored.size() && scored[tie_end].first == scored[k].first) {
                tie_end++;
            }
            for (int t = k; t < tie_end; ++t) {
                expert_ranks[i][scored[t].second] = rank;
            }
            rank += (tie_end - k);
            k = tie_end;
        }
    }
}
