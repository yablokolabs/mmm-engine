# ⚡ MMM Engine (C++23)

### **The Fastest Deterministic MMM Engine on Earth — Built with C++23**

## 🧩 Overview
**MMM Engine (C++23)** is an ultra-fast, deterministic Marketing Mix Modeling computation engine.
It performs:
- Adstock  
- Saturation  
- Trend & Seasonality  
- Channel contribution  
- Forecasting  
- Scenario simulation  
- Budget optimization  

All using **pure mathematical formulas**, with no machine learning, no Bayesian modeling, and no training loops.

It is designed to be:
- **Blazing fast** (microseconds to milliseconds)
- **Transparent & deterministic**
- **Embeddable** in enterprise systems
- **Open-source friendly**
- **Easy to integrate (JSON in → JSON out)**

This is the **fastest MMM computation engine available**, outperforming Robyn, Meridian, LightweightMMM, and all R/Python-based MMM tools.

---
## 🧮 Math Formulas

- **Geometric adstock:** $a_t = x_t + \lambda a_{t-1}$ with decay $\lambda \in [0,1)$.

- **Hill saturation:** $s_t = \frac{x_t^p}{\alpha^p + x_t^p + 10^{-9}}$ with slope $p$ and half-saturation $\alpha$.

- **Trend polynomial:** $g(t) = t_0 + t_1 t + t_2 t^2$.

- **Seasonality:** $h(t) = \sum_k a_k \cos\left(\frac{2\pi (k+1)t}{52}\right) + b_k \sin\left(\frac{2\pi (k+1)t}{52}\right)$.

- **Channel contribution:** $c_{t}^{(i)} = \beta_i \, s_{t}^{(i)}$ after adstock + saturation.

- **Prediction:** $y_t = g(t) + h(t) + \sum_i c_{t}^{(i)}$.

- **Budget optimizer:** Each step nudges spends $x_j \leftarrow x_j + \text{lr}$, then rescales so $\sum_j x_j = \text{total}$.

---
## 🚀 Key Features
- ⚡ **C++23, SIMD, mdspan, par_unseq**
- 📦 **JSON config input**
- 📤 **JSON output**
- 📈 **Deterministic forecasting**
- 🔁 **Parallel channel evaluation**
- ⏱ **Ultra-low latency (<1ms typical)**
- 🧮 **Pure mathematical MMM (no AI/ML)**
- 🧱 **Simple CLI interface**
- 📚 **Easy to extend or embed**
- 🖥️ **Cross-platform (Linux/macOS/Windows)**

---
## 🔍 What This Engine *Is*
- A **deterministic MMM simulator**
- A **formula-based forecasting engine**
- A **high-performance ROI curve executor**
- A **scenario planner**
- A **fast budgeting optimizer**
- A **transparent, predictable math engine**

## ❌ What This Engine *Is NOT*
- Not a machine learning model  
- Not Bayesian MMM  
- Not a regression tool  
- Not a model training framework  
- Not a replacement for Meta Robyn  
- Not an automated model builder  

It does **not** estimate coefficients from data.
You **provide all parameters manually**.

---
## 🧠 Why Deterministic MMM?
Because:
- ML MMM: minutes → hours  
- Bayesian MMM: hours → days  
- **This engine: < 1 millisecond**  

Perfect for:
- Large-scale experimentation  
- Real-time forecasting APIs  
- Embedded marketing optimization systems  
- Real world MMM at the speed of production

---
## 📦 Installation
```bash
git clone https://github.com/YOUR_GITHUB_USERNAME/mmm-engine
cd mmm-engine
mkdir build && cd build
cmake ..
make -j
```

## 🖥️ CLI Usage

Run MMM prediction:

```
./mmm run --config model.json --output out.json
```
Forecast N future weeks:
```
./mmm forecast --config model.json --weeks 26 --output forecast.json
```

## 📄 Example JSON Config
```
{

  "trend": { "t0": 100, "t1": 2, "t2": 0 },
  "seasonality": {
    "a": [10],
    "b": [5]
  },
  "channels": [
    {
      "name": "Facebook",
      "params": { "beta": 0.8, "alpha": 200, "p": 1.5, "decay": 0.4 },
      "spend": [100,120,130,150,140,135,150,160]
    }
  ]
}
```

## 📁 Examples
See:
```
/examples/basic
```

Run:

```
cd examples/basic
./run.sh
```

## 📂 Project Structure

```
mmm-engine/
├── include/
├── src/
├── examples/
├── CMakeLists.txt
└── README.md
```

## 🛠️ Build, Contributions & Extensibility

This project welcomes:
- PRs
- Issues
- Feature proposals
- Documentation improvements

If you wish to extend functionality (e.g., GPU kernels, additional MMM curves, custom optimizers), the architecture is explicitly modular.

## 📜 License

Released under the MIT License — free for commercial & open-source use.

## ⭐ Support the Project

If this engine helps you:
- ⭐ Star the repo
- 📢 Share it
- 🛠 Contribute back

Together, we can build the fastest open-source MMM engine ecosystem.

---
