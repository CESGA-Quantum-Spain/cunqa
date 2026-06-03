#pragma once

// noise_model_from_properties.cpp
//
// Build an Qiskit-Aer "NoiseModel" dict (the same cunqa::JSON that
// `NoiseModel.from_backend(...).to_dict(serializable=True)` produces in the
// CUNQA Python pipeline) directly from a `noise_properties` cunqa::JSON file, in C++.
//
// It reproduces, without Qiskit:
//   * basic_device_readout_errors  -> roerror entries
//   * basic_device_gate_errors     -> qerror entries
//       (depolarizing_error  followed by  thermal_relaxation_error)
//
// Physics is taken verbatim from qiskit-aer 0.17.x:
//   qiskit_aer/noise/device/models.py
//   qiskit_aer/noise/errors/standard_errors.py
//   qiskit_aer/noise/errors/quantum_error.py  (serialization)
//   qiskit_aer/noise/noise_model.py           (to_dict / AerJSONEncoder)
//
// Build:   g++ -O2 -std=c++17 -c noise_model_from_properties.cpp
//
// Entry point:
//   cunqa::JSON build_aer_noise_json_from_properties(
//       const cunqa::JSON& noise_properties,   // parsed `noise_properties` object
//       bool thermal_relaxation,        // include thermal-relaxation errors
//       bool readout_error,             // include readout (roerror) errors
//       bool gate_error);               // include depolarizing gate errors
//   -> returns the Aer NoiseModel cunqa::JSON {"errors":[...]}.
//
//   Qubit temperature is fixed at 1 mK (== temperature=True in
//   noise_instructions.py).
//

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "utils/json.hpp"

#include "logger.hpp"



// ---------------------------------------------------------------------------
// Constants matching qiskit-aer
// ---------------------------------------------------------------------------
static constexpr double KRAUS_ATOL = 1e-10;  // eigenvalue cutoff for Choi->Kraus

// ---------------------------------------------------------------------------
// Small utilities
// ---------------------------------------------------------------------------
static std::string random_hex32() {
    static std::mt19937_64 rng(std::random_device{}());
    static const char* hexd = "0123456789abcdef";
    std::string s(32, '0');
    for (int i = 0; i < 32; ++i) s[i] = hexd[rng() & 0xF];
    return s;
}

static int qubit_index(const std::string& s) {
    // "q[<n>]" -> n
    static const std::regex re(R"(q\[(\d+)\])");
    std::smatch m;
    if (std::regex_search(s, m, re)) return std::stoi(m[1].str());
    throw std::runtime_error("Invalid qubit string format: " + s);
}

// Supported gate name set (mirrors cunqabackend._get_gate).
static const std::set<std::string>& supported_gates() {
    static const std::set<std::string> g = {
        "id","x","y","z","h","s","sdg","sx","sxdg","t","tdg","swap","cx","cy",
        "cz","csx","ccx","ccz","cswap","ecr","reset",
        "u1","u2","u3","cu1","cu3","u","cu","p","r","rx","ry","rz",
        "crx","cry","crz","rxx","ryy","rzz","rzx","cp"};
    return g;
}

// ---------------------------------------------------------------------------
// A single error "term": a list of instructions + an (unnormalized) probability
// ---------------------------------------------------------------------------
struct Term {
    cunqa::JSON instructions = cunqa::JSON::array();  // list of {"name":..,"qubits":[..],("params":..)}
    double prob = 0.0;
};

static cunqa::JSON gate_inst(const std::string& name, std::vector<int> qubits) {
    cunqa::JSON j;
    j["name"] = name;
    j["qubits"] = qubits;
    return j;
}

// Drop non-positive-probability terms and renormalize the rest to sum 1.
// (Mirrors the QuantumError constructor: it filters prob>0 then normalizes.)
static void clean_normalize(std::vector<Term>& terms) {
    std::vector<Term> kept;
    double tot = 0.0;
    for (auto& t : terms)
        if (t.prob > 0.0) { tot += t.prob; kept.push_back(std::move(t)); }
    if (kept.empty()) throw std::runtime_error("error has no positive-probability terms");
    for (auto& t : kept) t.prob /= tot;
    terms = std::move(kept);
}

// ---------------------------------------------------------------------------
// Real-symmetric 4x4 Jacobi eigensolver (the thermal-relaxation Choi matrix is
// always real symmetric).  Returns eigenvalues w[] and eigenvectors as columns
// of V (V[row][col]).
// ---------------------------------------------------------------------------
static void jacobi_eigh_4(double A[4][4], double w[4], double V[4][4]) {
    const int n = 4;
    double a[4][4];
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) { a[i][j] = A[i][j]; V[i][j] = (i == j) ? 1.0 : 0.0; }

    for (int sweep = 0; sweep < 100; ++sweep) {
        double off = 0.0;
        for (int p = 0; p < n; ++p)
            for (int q = p + 1; q < n; ++q) off += a[p][q] * a[p][q];
        if (off < 1e-30) break;
        for (int p = 0; p < n; ++p) {
            for (int q = p + 1; q < n; ++q) {
                if (std::fabs(a[p][q]) < 1e-300) continue;
                double theta = (a[q][q] - a[p][p]) / (2.0 * a[p][q]);
                double t = (theta >= 0 ? 1.0 : -1.0) /
                           (std::fabs(theta) + std::sqrt(theta * theta + 1.0));
                double c = 1.0 / std::sqrt(t * t + 1.0);
                double s = t * c;
                for (int k = 0; k < n; ++k) {
                    double akp = a[k][p], akq = a[k][q];
                    a[k][p] = c * akp - s * akq;
                    a[k][q] = s * akp + c * akq;
                }
                for (int k = 0; k < n; ++k) {
                    double apk = a[p][k], aqk = a[q][k];
                    a[p][k] = c * apk - s * aqk;
                    a[q][k] = s * apk + c * aqk;
                }
                for (int k = 0; k < n; ++k) {
                    double vkp = V[k][p], vkq = V[k][q];
                    V[k][p] = c * vkp - s * vkq;
                    V[k][q] = s * vkp + c * vkq;
                }
            }
        }
    }
    for (int i = 0; i < n; ++i) w[i] = a[i][i];
}

// ---------------------------------------------------------------------------
// Thermal-relaxation single-qubit error (qiskit standard_errors.thermal_relaxation_error)
//   local_q : local qubit index used inside the instruction list
// Returns the list of terms; also sets out_Fpro to the channel process fidelity
// F_pro = (2 - p_reset + 2*exp_t2)/4   (validated to match average_gate_fidelity).
// ---------------------------------------------------------------------------
static std::vector<Term> thermal_relaxation_terms(double t1, double t2, double time,
                                                  double population, int local_q,
                                                  double& out_Fpro) {
    // t2 truncated to 2*t1 already by caller, but guard again.
    if (t2 > 2.0 * t1) t2 = 2.0 * t1;

    double rate1 = 1.0 / t1, rate2 = 1.0 / t2;
    double p_reset = 1.0 - std::exp(-time * rate1);
    double exp_t2  = std::exp(-time * rate2);
    double p0 = 1.0 - population, p1 = population;

    out_Fpro = (2.0 - p_reset + 2.0 * exp_t2) / 4.0;

    std::vector<Term> terms;
    if (t2 <= t1) {
        // Probabilistic mixture: [I, Z, Reset->|0>, Reset->|1>]
        double p_reset0 = p_reset * p0;
        double p_reset1 = p_reset * p1;
        double p_z = (1.0 - p_reset) * (1.0 - std::exp(-time * (rate2 - rate1))) / 2.0;
        double p_id = 1.0 - p_z - p_reset0 - p_reset1;

        { Term t; t.instructions.push_back(gate_inst("id",    {local_q})); t.prob = p_id;    terms.push_back(t); }
        { Term t; t.instructions.push_back(gate_inst("z",     {local_q})); t.prob = p_z;     terms.push_back(t); }
        { Term t; t.instructions.push_back(gate_inst("reset", {local_q})); t.prob = p_reset0; terms.push_back(t); }
        { Term t; t.instructions.push_back(gate_inst("reset", {local_q}));
                  t.instructions.push_back(gate_inst("x",     {local_q})); t.prob = p_reset1; terms.push_back(t); }
    } else {
        // T1 < T2 <= 2*T1 : general Kraus channel from the Choi matrix
        //   [[1-p1*pr, 0, 0, e2 ],
        //    [0, p1*pr, 0, 0   ],
        //    [0, 0, p0*pr, 0   ],
        //    [e2, 0, 0, 1-p0*pr]]
        double M[4][4] = {{0}};
        M[0][0] = 1.0 - p1 * p_reset;
        M[1][1] = p1 * p_reset;
        M[2][2] = p0 * p_reset;
        M[3][3] = 1.0 - p0 * p_reset;
        M[0][3] = M[3][0] = exp_t2;

        double w[4], V[4][4];
        jacobi_eigh_4(M, w, V);

        // Sort eigen-pairs by DESCENDING eigenvalue (largest Kraus first).
        std::array<int,4> idx = {0,1,2,3};
        std::sort(idx.begin(), idx.end(), [&](int a, int b){ return w[a] > w[b]; });

        cunqa::JSON params = cunqa::JSON::array();   // list of Kraus matrices
        for (int ii : idx) {
            double lam = w[ii];
            if (lam <= KRAUS_ATOL) continue;
            double sq = std::sqrt(lam);
            // K = sqrt(lam) * unstack(vec)  (column-major / order='F'):
            //   K[r][c] = vec[r + 2c]
            double k[2][2];
            k[0][0] = sq * V[0][ii];
            k[1][0] = sq * V[1][ii];
            k[0][1] = sq * V[2][ii];
            k[1][1] = sq * V[3][ii];
            cunqa::JSON mat = cunqa::JSON::array();
            for (int r = 0; r < 2; ++r) {
                cunqa::JSON row = cunqa::JSON::array();
                for (int c = 0; c < 2; ++c)
                    row.push_back(cunqa::JSON::array({k[r][c], 0.0}));  // complex -> [re, im]
                mat.push_back(row);
            }
            params.push_back(mat);
        }
        Term t;
        cunqa::JSON kr = gate_inst("kraus", {local_q});
        kr["params"] = params;
        t.instructions.push_back(kr);
        t.prob = 1.0;
        terms.push_back(t);
    }
    clean_normalize(terms);
    return terms;
}

// excited-state population from qubit frequency [Hz] and temperature [mK]
// (qiskit models._excited_population)
static double excited_population(double freq, double temperature) {
    if (temperature == 0.0) return 0.0;
    double exp_param = std::exp((47.99243e-9 * freq) / std::fabs(temperature));
    double population = 1.0 / (1.0 + exp_param);
    if (temperature < 0.0) population = 1.0 - population;
    return population;
}

// ---------------------------------------------------------------------------
// Depolarizing error terms (qiskit standard_errors.depolarizing_error)
//   num_qubits == 1 -> single-qubit gates id/x/y/z
//   num_qubits >= 2 -> single "pauli" instruction with a label
// Pauli enumeration follows itertools.product("IXYZ", repeat=n), identity first.
// ---------------------------------------------------------------------------
static std::vector<Term> depolarizing_terms(double param, int num_qubits,
                                            const std::vector<int>& local_qubits) {
    double num_terms = std::pow(4.0, num_qubits);
    double max_param = num_terms / (num_terms - 1.0);
    double prob_iden  = 1.0 - param / max_param;
    double prob_pauli = param / num_terms;

    const char paul[4] = {'I', 'X', 'Y', 'Z'};
    int total = static_cast<int>(num_terms);

    std::vector<Term> terms;
    for (int idx = 0; idx < total; ++idx) {
        // decode idx into base-4 label (most significant char first)
        std::string label(num_qubits, 'I');
        int v = idx;
        for (int pos = num_qubits - 1; pos >= 0; --pos) { label[pos] = paul[v & 3]; v >>= 2; }
        bool is_iden = (idx == 0);

        Term t;
        t.prob = is_iden ? prob_iden : prob_pauli;
        if (num_qubits == 1) {
            char ch = label[0];
            std::string nm = (ch == 'I') ? "id" : std::string(1, std::tolower(ch));
            t.instructions.push_back(gate_inst(nm, {local_qubits[0]}));
        } else {
            cunqa::JSON inst = gate_inst("pauli", local_qubits);
            inst["params"] = cunqa::JSON::array({label});
            t.instructions.push_back(inst);
        }
        terms.push_back(std::move(t));
    }
    clean_normalize(terms);  // identity term may be 0 (param==max_param) etc.
    return terms;
}

// compose(depol, relax): depol applied first, relax second.
//   instructions = depol.instr ++ relax.instr   ;   prob = pd * pr
// (matches QuantumError.compose: outer loop self/depol, inner loop other/relax)
static std::vector<Term> compose(const std::vector<Term>& depol,
                                 const std::vector<Term>& relax) {
    std::vector<Term> out;
    out.reserve(depol.size() * relax.size());
    for (const auto& d : depol)
        for (const auto& r : relax) {
            Term t;
            t.instructions = d.instructions;
            for (const auto& inst : r.instructions) t.instructions.push_back(inst);
            t.prob = d.prob * r.prob;
            out.push_back(std::move(t));
        }
    return out;
}

// tensor of per-qubit relaxation term-lists into a multi-qubit relaxation error.
// Each per-qubit term list already uses its own local qubit index.
static std::vector<Term> tensor_relax(const std::vector<std::vector<Term>>& per_qubit) {
    std::vector<Term> acc;
    acc.push_back(Term{cunqa::JSON::array(), 1.0});
    for (const auto& q_terms : per_qubit) {
        std::vector<Term> next;
        for (const auto& a : acc)
            for (const auto& b : q_terms) {
                Term t;
                t.instructions = a.instructions;
                for (const auto& inst : b.instructions) t.instructions.push_back(inst);
                t.prob = a.prob * b.prob;
                next.push_back(std::move(t));
            }
        acc = std::move(next);
    }
    return acc;
}

// ---------------------------------------------------------------------------
// Build one qerror dict for a gate acting on `qubits`, with relaxation params
// per (local) qubit and a reported gate error_param.
// ---------------------------------------------------------------------------
struct RelaxParam { double t1, t2, freq; };

static bool build_gate_error(const std::string& name,
                             const std::vector<int>& qubits,
                             double gate_time, double error_param,
                             const std::vector<RelaxParam>& relax_params,
                             bool thermal_relaxation, bool gate_error,
                             double temperature, cunqa::JSON& out_error) {
    int n = static_cast<int>(qubits.size());

    // ---- relaxation error + its average gate fidelity ----
    std::vector<Term> relax_terms;
    bool have_relax = false;
    double relax_fid = 1.0, relax_infid = 0.0;

    if (thermal_relaxation && gate_time > 0.0) {
        std::vector<std::vector<Term>> per_qubit;
        double Fpro_total = 1.0;
        for (int j = 0; j < n; ++j) {
            double t1 = relax_params[j].t1;
            double t2 = std::min(relax_params[j].t2, 2.0 * t1);
            double pop = excited_population(relax_params[j].freq, temperature);
            double Fpro = 1.0;
            per_qubit.push_back(thermal_relaxation_terms(t1, t2, gate_time, pop, j, Fpro));
            Fpro_total *= Fpro;
        }
        relax_terms = tensor_relax(per_qubit);
        clean_normalize(relax_terms);
        have_relax = true;
        double dim = std::pow(2.0, n);
        relax_fid = (dim * Fpro_total + 1.0) / (dim + 1.0);
        relax_infid = 1.0 - relax_fid;
    }

    // ---- depolarizing error (only if the reported error exceeds the
    //      coherence-limited infidelity)  qiskit _device_depolarizing_error ----
    std::vector<Term> depol_terms;
    bool have_depol = false;
    if (gate_error && error_param > relax_infid) {
        double dim = std::pow(2.0, n);
        double error_max = dim / (dim + 1.0);
        double e = std::min(error_param, error_max);
        double depol_param = dim * (e - relax_infid) / (dim * relax_fid - 1.0);
        double max_param = std::pow(4.0, n) / (std::pow(4.0, n) - 1.0);
        if (depol_param > max_param) depol_param = std::min(depol_param, max_param);
        if (depol_param > 0.0) {
            depol_terms = depolarizing_terms(depol_param, n, qubits.size() == 1
                                                 ? std::vector<int>{0}
                                                 : [&]{ std::vector<int> v(n); for (int j=0;j<n;++j) v[j]=j; return v; }());
            have_depol = true;
        }
    }

    // ---- combine ----
    std::vector<Term> combined;
    if (have_depol && have_relax)      combined = compose(depol_terms, relax_terms);
    else if (have_depol)               combined = depol_terms;
    else if (have_relax)               combined = relax_terms;
    else                               return false;  // no error for this gate

    clean_normalize(combined);

    cunqa::JSON instructions = cunqa::JSON::array();
    cunqa::JSON probabilities = cunqa::JSON::array();
    for (const auto& t : combined) {
        instructions.push_back(t.instructions);
        probabilities.push_back(t.prob);
    }

    out_error = cunqa::JSON::object();
    out_error["type"] = "qerror";
    out_error["id"] = random_hex32();
    out_error["operations"] = cunqa::JSON::array({name});
    out_error["instructions"] = instructions;
    out_error["probabilities"] = probabilities;
    out_error["gate_qubits"] = cunqa::JSON::array({qubits});
    return true;
}

// ---------------------------------------------------------------------------
// Public entry point.
//
// Build the Qiskit-Aer NoiseModel cunqa::JSON (the object that
// `NoiseModel.from_backend(...).to_dict(serializable=True)` produces) directly
// from a CUNQA `noise_properties` cunqa::JSON object.
//
//   noise_properties   : parsed `noise_properties` object ("Qubits",
//                        "Q1Gates", "Q2Gates(RB)")
//   thermal_relaxation : include thermal-relaxation errors
//   readout_error      : include readout (roerror) errors
//   gate_error         : include depolarizing gate errors
//
// Returns a cunqa::JSON object of the form {"errors": [ ... ]}.
// Qubit temperature is fixed at 1 mK, mirroring `temperature=True` in
// noise_instructions.py.  Unsupported gate names are skipped with a stderr
// warning (mirrors cunqabackend._get_gate).
// ---------------------------------------------------------------------------
cunqa::JSON build_aer_noise_model(
    const cunqa::JSON& noise_properties,
    bool thermal_relaxation,
    bool readout_error,
    bool gate_error
) {
    const double temperature = 1.0;  // mK, == temperature=True
    const cunqa::JSON& props = noise_properties;

    cunqa::JSON errors = cunqa::JSON::array();

    // -- per-qubit relaxation parameters + readout errors --
    std::map<int, RelaxParam> qparam;
    {
        const auto& qubits = props.at("Qubits");
        for (auto it = qubits.begin(); it != qubits.end(); ++it) {
            int q = qubit_index(it.key());
            const cunqa::JSON& v = it.value();
            qparam[q] = RelaxParam{ v.at("T1 (s)").get<double>(),
                                    v.at("T2 (s)").get<double>(),
                                    v.at("Drive Frequency (Hz)").get<double>() };
            if (readout_error) {
                double e = 1.0 - v.at("Readout fidelity (RB)").get<double>();
                cunqa::JSON ro;
                ro["type"] = "roerror";
                ro["operations"] = cunqa::JSON::array({"measure"});
                ro["probabilities"] = cunqa::JSON::array({ cunqa::JSON::array({1.0 - e, e}),
                                                    cunqa::JSON::array({e, 1.0 - e}) });
                ro["gate_qubits"] = cunqa::JSON::array({ cunqa::JSON::array({q}) });
                errors.push_back(ro);
            }
        }
    }

    auto relax_for = [&](const std::vector<int>& qs) {
        std::vector<RelaxParam> rp;
        for (int q : qs) rp.push_back(qparam.at(q));
        return rp;
    };

    // -- single-qubit gate errors --
    if (props.contains("Q1Gates")) {
        for (auto qit = props["Q1Gates"].begin(); qit != props["Q1Gates"].end(); ++qit) {
            int q = qubit_index(qit.key());
            for (auto git = qit.value().begin(); git != qit.value().end(); ++git) {
                std::string name = git.key();
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                if (!supported_gates().count(name)) {
                    std::cerr << "[warn] gate '" << name << "' not supported by Aer; skipped.\n";
                    continue;
                }
                const cunqa::JSON& gp = git.value();
                double dur = gp.at("Gate duration (s)").get<double>();
                double err = 1.0 - gp.at("Fidelity(RB)").get<double>();
                cunqa::JSON e;
                if (build_gate_error(name, {q}, dur, err, relax_for({q}),
                                     thermal_relaxation, gate_error, temperature, e))
                    errors.push_back(e);
            }
        }
    }

    // -- two-qubit gate errors --
    if (props.contains("Q2Gates(RB)")) {
        for (auto pit = props["Q2Gates(RB)"].begin(); pit != props["Q2Gates(RB)"].end(); ++pit) {
            for (auto git = pit.value().begin(); git != pit.value().end(); ++git) {
                std::string name = git.key();
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                if (!supported_gates().count(name)) {
                    std::cerr << "[warn] gate '" << name << "' not supported by Aer; skipped.\n";
                    continue;
                }
                const cunqa::JSON& gp = git.value();
                int ctrl = gp.at("Control").get<int>();
                int tgt  = gp.at("Target").get<int>();
                double dur = gp.at("Duration (s)").get<double>();
                double err = 1.0 - gp.at("Fidelity(RB)").get<double>();
                cunqa::JSON e;
                if (build_gate_error(name, {ctrl, tgt}, dur, err, relax_for({ctrl, tgt}),
                                     thermal_relaxation, gate_error, temperature, e))
                    errors.push_back(e);
            }
        }
    }

    return cunqa::JSON{{"errors", errors}};
}
