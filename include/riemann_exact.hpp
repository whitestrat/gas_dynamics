#ifndef RIEMANN_EXACT_HPP
#define RIEMANN_EXACT_HPP

// Точный решатель задачи Римана, C++17.
// Основа: sem1-14-slides.pdf, слайды 2, 4, 7, 9-15.
// Файл не содержит main, ввода-вывода и сведений о сетке.
// Подключение: #include "riemann_exact.hpp"
// Использование:
//   auto r = riemann::solve({1, 0, 1}, {0.125, 0, 0.1});
//   if (r.ok()) { auto w = riemann::sample(r, 0.0); }
// Все физические обозначения взяты из слайдов; *_star означает индекс *.
// При ошибке проверяйте status/message; остальные поля не являются решением.

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>

namespace riemann {

// Примитивные W, консервативные Q и поток F: слайды 2 и 11.
struct W {
    double rho = 0.0;
    double u = 0.0;
    double p = 0.0;
};

struct Q {
    double rho = 0.0;
    double rho_u = 0.0;
    double rho_E = 0.0;
};

struct F {
    double rho_u = 0.0;
    double rho_u2_p = 0.0;
    double rhoE_p_u = 0.0;
};

struct Parameters {
    double e = 0.0;  // Удельная внутренняя энергия.
    double E = 0.0;  // Удельная полная энергия.
    double a = 0.0;  // Скорость звука.
    double s = 0.0;  // Энтропийная функция p / rho^gamma.
};

inline bool valid(const W& w) {
    return std::isfinite(w.rho) && w.rho > 0.0 &&
           std::isfinite(w.p) && w.p > 0.0 && std::isfinite(w.u);
}

// Эти вспомогательные функции вызываются для допустимых W и gamma > 1.
inline Parameters parameters(const W& w, double gamma) {
    const double e = w.p / ((gamma - 1.0) * w.rho);
    return {e, e + 0.5 * w.u * w.u,
            std::sqrt(gamma * w.p / w.rho), w.p / std::pow(w.rho, gamma)};
}

inline Q conservative(const W& w, double gamma) {
    return {w.rho, w.rho * w.u, w.rho * parameters(w, gamma).E};
}

inline F flux(const W& w, double gamma) {
    const Q q = conservative(w, gamma);
    return {q.rho_u, q.rho_u * w.u + w.p, (q.rho_E + w.p) * w.u};
}

enum class Status { success, invalid_input, vacuum, no_convergence, numerical_error };
enum class WaveType { shock, rarefaction };
enum class GuessType { linear, two_rarefactions, two_shocks };

inline const char* name(WaveType type) {
    return type == WaveType::shock ? "shock" : "rarefaction";
}

inline const char* name(GuessType type) {
    if (type == GuessType::linear) return "linear";
    if (type == GuessType::two_rarefactions) return "two_rarefactions";
    return "two_shocks";
}

// Все результаты одного вызова; никаких общих изменяемых переменных нет.
struct Solution {
    Status status = Status::invalid_input;
    std::string message;
    W W_L, W_R, W_starL, W_starR, W_0;
    F F_0;
    double gamma = 1.4;
    double p_star = 0.0, u_star = 0.0;
    double rho_starL = 0.0, rho_starR = 0.0;
    WaveType left = WaveType::rarefaction, right = WaveType::rarefaction;
    // Для УВ используются S_L/S_R, для ВР - S_HL/S_TL/S_HR/S_TR.
    // Неиспользуемые скорости равны 0 и не имеют физического смысла.
    double S_L = 0.0, S_R = 0.0;
    double S_HL = 0.0, S_TL = 0.0, S_HR = 0.0, S_TR = 0.0;
    double p_lin = 0.0, p_initial = 0.0;
    GuessType guess = GuessType::linear;
    int iterations = 0, damped_steps = 0;
    double relative_change = 0.0, residual = 0.0;
    bool ok() const { return status == Status::success; }
};

// f(p; W_K) и производная по p. Предусловие: p > 0, W_K допустимо.
// УВ: соотношения Ранкина-Гюгонио. ВР: изоэнтропическое соотношение.
// Буквы A, B, a соответствуют слайду 7.
struct FunctionValue { double f, df; };

inline FunctionValue f(double p, const W& W_K, double gamma) {
    if (p > W_K.p) {
        const double A = 2.0 / ((gamma + 1.0) * W_K.rho);
        const double B = (gamma - 1.0) / (gamma + 1.0) * W_K.p;
        const double factor = std::sqrt(A / (p + B));
        return {(p - W_K.p) * factor,
                factor * (1.0 - (p - W_K.p) / (2.0 * (p + B)))};
    }
    const double a = parameters(W_K, gamma).a;
    const double z = (gamma - 1.0) / (2.0 * gamma);
    // expm1 повышает точность f вблизи p = p_K.
    return {2.0 * a / (gamma - 1.0) * std::expm1(z * std::log(p / W_K.p)),
            std::pow(p / W_K.p, -(gamma + 1.0) / (2.0 * gamma)) /
                (W_K.rho * a)};
}

// GUESSP: формулы и порядок выбора в точности со слайдов 9-10.
// Акустическое p_lin берётся через rho_L*a_L и rho_R*a_R.
inline void guess_pressure(Solution& r) {
    const W& W_L = r.W_L;
    const W& W_R = r.W_R;
    const double gamma = r.gamma;
    const double a_L = parameters(W_L, gamma).a;
    const double a_R = parameters(W_R, gamma).a;
    const double rho_a_L = W_L.rho * a_L;
    const double rho_a_R = W_R.rho * a_R;
    r.p_lin = (rho_a_R * W_L.p + rho_a_L * W_R.p +
               rho_a_L * rho_a_R * (W_L.u - W_R.u)) / (rho_a_L + rho_a_R);
    const double p_min = std::min(W_L.p, W_R.p);
    const double p_max = std::max(W_L.p, W_R.p);

    if (p_max / p_min <= 2.0 && p_min <= r.p_lin && r.p_lin <= p_max) {
        r.guess = GuessType::linear;
        r.p_initial = r.p_lin;
    } else if (r.p_lin < p_min) {
        r.guess = GuessType::two_rarefactions;
        const double z = (gamma - 1.0) / (2.0 * gamma);
        r.p_initial = std::pow(
            (a_L + a_R - 0.5 * (gamma - 1.0) * (W_R.u - W_L.u)) /
            (a_L / std::pow(W_L.p, z) + a_R / std::pow(W_R.p, z)), 1.0 / z);
    } else {
        r.guess = GuessType::two_shocks;
        const double p_hat = r.p_lin;
        const double m_L = std::sqrt(W_L.rho *
            ((gamma + 1.0) * p_hat + (gamma - 1.0) * W_L.p) / 2.0);
        const double m_R = std::sqrt(W_R.rho *
            ((gamma + 1.0) * p_hat + (gamma - 1.0) * W_R.p) / 2.0);
        r.p_initial = (m_L * m_R * (W_L.u - W_R.u) +
                       m_R * W_L.p + m_L * W_R.p) / (m_L + m_R);
    }
}

// Плотность за волной: формулы для УВ/ВР на слайде 7.
inline double star_density(double p_star, const W& W_K, double gamma) {
    if (p_star > W_K.p) {
        return W_K.rho * ((gamma + 1.0) * p_star + (gamma - 1.0) * W_K.p) /
               ((gamma - 1.0) * p_star + (gamma + 1.0) * W_K.p);
    }
    return W_K.rho * std::pow(p_star / W_K.p, 1.0 / gamma);
}

// Отдельная функция отбора W(S), S = (x-x0)/t; слайды 11-13.
// При S = u_star выбирается левое значение контактного разрыва.
// На скачке функция возвращает одно из односторонних значений.
// nullopt означает недопустимый S или неуспешное решение.
inline std::optional<W> sample(const Solution& r, double S) {
    if (!r.ok() || !std::isfinite(S)) return std::nullopt;
    const double gamma = r.gamma;
    if (S <= r.u_star) {
        if (r.left == WaveType::shock) return S <= r.S_L ? r.W_L : r.W_starL;
        if (S <= r.S_HL) return r.W_L;
        if (S >= r.S_TL) return r.W_starL;

        // Внутри левого веера: c = a/a_L, как на слайде 13.
        const double a_L = parameters(r.W_L, gamma).a;
        const double c = 2.0 / (gamma + 1.0) +
            (gamma - 1.0) / ((gamma + 1.0) * a_L) * (r.W_L.u - S);
        const double u = 2.0 / (gamma + 1.0) *
            (a_L + 0.5 * (gamma - 1.0) * r.W_L.u + S);
        return W{r.W_L.rho * std::pow(c, 2.0 / (gamma - 1.0)), u,
                 r.W_L.p * std::pow(c, 2.0 * gamma / (gamma - 1.0))};
    }
    if (r.right == WaveType::shock) return S >= r.S_R ? r.W_R : r.W_starR;
    if (S >= r.S_HR) return r.W_R;
    if (S <= r.S_TR) return r.W_starR;

    // Правый веер получается зеркальным отражением левого.
    const double a_R = parameters(r.W_R, gamma).a;
    const double c = 2.0 / (gamma + 1.0) -
        (gamma - 1.0) / ((gamma + 1.0) * a_R) * (r.W_R.u - S);
    const double u = 2.0 / (gamma + 1.0) *
        (-a_R + 0.5 * (gamma - 1.0) * r.W_R.u + S);
    return W{r.W_R.rho * std::pow(c, 2.0 / (gamma - 1.0)), u,
             r.W_R.p * std::pow(c, 2.0 * gamma / (gamma - 1.0))};
}

// Основной интерфейс: на входе только W_L, W_R, gamma и точность Ньютона.
// Ошибки, включая вакуум, возвращаются через status/message без exit/abort.
inline Solution solve(const W& W_L, const W& W_R, double gamma = 1.4,
                      double eps = 1e-6, int max_iterations = 100) {
    Solution r;
    r.W_L = W_L;
    r.W_R = W_R;
    r.gamma = gamma;
    if (!valid(W_L) || !valid(W_R) || !std::isfinite(gamma) || gamma <= 1.0 ||
        !std::isfinite(eps) || eps <= 0.0 || eps >= 1.0 || max_iterations <= 0) {
        r.message = "Нужны конечные rho>0, p>0, u, gamma>1, 0<eps<1 и max_iterations>0.";
        return r;
    }
    const double a_L = parameters(W_L, gamma).a;
    const double a_R = parameters(W_R, gamma).a;
    if (!std::isfinite(a_L) || !std::isfinite(a_R) || a_L <= 0.0 || a_R <= 0.0) {
        r.status = Status::numerical_error;
        r.message = "Скорость звука вне представимого диапазона double.";
        return r;
    }

    // При равенстве тоже нет решения с p_star > 0: предельный вакуум.
    if (W_R.u - W_L.u >= 2.0 * (a_L + a_R) / (gamma - 1.0)) {
        r.status = Status::vacuum;
        r.message = "Вакуум: u_R-u_L >= 2(a_L+a_R)/(gamma-1). Решение не строится.";
        return r;
    }

    guess_pressure(r);
    if (!std::isfinite(r.p_initial) || r.p_initial <= 0.0) {
        r.status = Status::numerical_error;
        r.message = "GUESSP: положительное давление не представимо в double.";
        return r;
    }

    // Метод Ньютона. Если шаг даёт p_next<=0, берём p_next=p_previous/2.
    // Таким образом, каждая оценка f выполняется при строго положительном p.
    // Абсолютного ограничения p>=1e-6 нет: оно испортило бы малые давления.
    double p = r.p_initial;
    bool converged = false;
    for (int i = 1; i <= max_iterations; ++i) {
        const FunctionValue f_L = f(p, W_L, gamma), f_R = f(p, W_R, gamma);
        const double residual = f_L.f + f_R.f + (W_R.u - W_L.u);
        const double df = f_L.df + f_R.df;
        if (!std::isfinite(residual) || !std::isfinite(df) || df <= 0.0) break;
        double p_next = p - residual / df;
        if (!std::isfinite(p_next)) break;
        if (p_next <= 0.0) {
            p_next = 0.5 * p;
            ++r.damped_steps;
        }
        if (p_next <= 0.0) break;
        r.iterations = i;
        r.relative_change = 2.0 * std::abs(p_next - p) / (p_next + p);
        p = p_next;
        if (r.relative_change < eps) { converged = true; break; }
    }
    if (!converged) {
        r.status = Status::no_convergence;
        r.message = "Ньютон не сошёлся или достиг предела double; увеличьте max_iterations/проверьте данные.";
        return r;
    }

    // Состояния по обе стороны контакта: p и u общие, rho различаются.
    const double f_L = f(p, W_L, gamma).f, f_R = f(p, W_R, gamma).f;
    r.p_star = p;
    r.u_star = 0.5 * (W_L.u + W_R.u) + 0.5 * (f_R - f_L);
    r.rho_starL = star_density(p, W_L, gamma);
    r.rho_starR = star_density(p, W_R, gamma);
    r.W_starL = {r.rho_starL, r.u_star, r.p_star};
    r.W_starR = {r.rho_starR, r.u_star, r.p_star};
    r.residual = f_L + f_R + (W_R.u - W_L.u);

    // Типы и скорости волн; индекс H - голова, T - хвост (слайд 13).
    r.left = p > W_L.p ? WaveType::shock : WaveType::rarefaction;
    r.right = p > W_R.p ? WaveType::shock : WaveType::rarefaction;
    if (r.left == WaveType::shock) {
        r.S_L = W_L.u - a_L * std::sqrt((gamma + 1.0) / (2.0 * gamma) *
                                      p / W_L.p + (gamma - 1.0) / (2.0 * gamma));
    } else {
        r.S_HL = W_L.u - a_L;
        r.S_TL = r.u_star - a_L * std::pow(p / W_L.p, (gamma - 1.0) / (2.0 * gamma));
    }
    if (r.right == WaveType::shock) {
        r.S_R = W_R.u + a_R * std::sqrt((gamma + 1.0) / (2.0 * gamma) *
                                      p / W_R.p + (gamma - 1.0) / (2.0 * gamma));
    } else {
        r.S_HR = W_R.u + a_R;
        r.S_TR = r.u_star + a_R * std::pow(p / W_R.p, (gamma - 1.0) / (2.0 * gamma));
    }

    // Проверяем числовой диапазон, затем отбираем W(0) и поток Годунова F(W(0)).
    if (!valid(r.W_starL) || !valid(r.W_starR) || !std::isfinite(r.residual) ||
        !std::isfinite(r.S_L) || !std::isfinite(r.S_R) ||
        !std::isfinite(r.S_HL) || !std::isfinite(r.S_TL) ||
        !std::isfinite(r.S_HR) || !std::isfinite(r.S_TR)) {
        r.status = Status::numerical_error;
        r.message = "Параметры решения вне представимого диапазона double.";
        return r;
    }
    r.status = Status::success;
    r.W_0 = *sample(r, 0.0);
    r.F_0 = flux(r.W_0, gamma);
    const Parameters v = parameters(r.W_0, gamma);
    if (!valid(r.W_0) || !std::isfinite(v.e) || !std::isfinite(v.E) ||
        !std::isfinite(v.a) || !std::isfinite(v.s) ||
        !std::isfinite(r.F_0.rho_u) || !std::isfinite(r.F_0.rho_u2_p) ||
        !std::isfinite(r.F_0.rhoE_p_u)) {
        r.status = Status::numerical_error;
        r.message = "Состояние/поток при S=0 вне представимого диапазона double.";
        return r;
    }
    r.message = "Решение найдено.";
    return r;
}

} // namespace riemann
#endif
