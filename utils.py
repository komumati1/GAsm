import numpy as np
from matplotlib import pyplot as plt
import json


def load_json_file(file_path):
    """Load and return the contents of a JSON file."""
    with open(file_path, 'r', encoding='utf-8') as file:
        data = json.load(file)
    return data


def plot_hist_from_json(hist_json: dict):
    entries = hist_json.get("entries", [])
    if not entries:
        raise ValueError("Brak 'entries' w JSON albo lista jest pusta.")

    def col(*keys):
        # pozwala obsłużyć różne nazwy pól (camelCase i snake_case)
        for k in keys:
            if k in entries[0]:
                return np.asarray([e[k] for e in entries], dtype=float)
        raise KeyError(f"Nie znaleziono żadnego z kluczy: {keys}")

    def remove_outliers_mask(data):
        Q1 = np.percentile(data, 25)
        Q3 = np.percentile(data, 75)
        IQR = Q3 - Q1
        lower_bound = Q1 - 1.5 * IQR
        upper_bound = Q3 + 1.5 * IQR
        return (data >= lower_bound) & (data <= upper_bound)

    gen      = col("generation", "gen")
    avg_fit  = col("avgFitness", "avg_fitness")
    best_fit = col("bestFitness", "best_fitness")
    avg_sz   = col("avgSize", "avg_size")

    fig, axes = plt.subplots(2, 3, figsize=(18, 10))

    # normalne
    axes[0, 0].plot(gen, avg_fit, label="Average Fitness", marker="o")
    axes[0, 0].set_xlabel("Generation"); axes[0, 0].set_ylabel("Fitness")
    axes[0, 0].set_title("Average Fitness over Generations")
    axes[0, 0].legend(); axes[0, 0].grid(True)

    axes[0, 1].plot(gen, best_fit, label="Best Fitness", marker="s")
    axes[0, 1].set_xlabel("Generation"); axes[0, 1].set_ylabel("Fitness")
    axes[0, 1].set_title("Best Fitness over Generations")
    axes[0, 1].legend(); axes[0, 1].grid(True)

    axes[0, 2].plot(gen, avg_sz, label="Average Size", color="orange", marker="^")
    axes[0, 2].set_xlabel("Generation"); axes[0, 2].set_ylabel("Average Size")
    axes[0, 2].set_title("Average Program Size over Generations")
    axes[0, 2].legend(); axes[0, 2].grid(True)

    # bez outlierów
    m = remove_outliers_mask(avg_fit)
    axes[1, 0].plot(gen[m], avg_fit[m], label="Average Fitness (no outliers)", marker="o")
    axes[1, 0].set_xlabel("Generation"); axes[1, 0].set_ylabel("Fitness")
    axes[1, 0].set_title("Average Fitness over Generations (No Outliers)")
    axes[1, 0].legend(); axes[1, 0].grid(True)

    m = remove_outliers_mask(best_fit)
    axes[1, 1].plot(gen[m], best_fit[m], label="Best Fitness (no outliers)", marker="s")
    axes[1, 1].set_xlabel("Generation"); axes[1, 1].set_ylabel("Fitness")
    axes[1, 1].set_title("Best Fitness over Generations (No Outliers)")
    axes[1, 1].legend(); axes[1, 1].grid(True)

    m = remove_outliers_mask(avg_sz)
    axes[1, 2].plot(gen[m], avg_sz[m], label="Average Size (no outliers)", color="orange", marker="^")
    axes[1, 2].set_xlabel("Generation"); axes[1, 2].set_ylabel("Average Size")
    axes[1, 2].set_title("Average Program Size over Generations (No Outliers)")
    axes[1, 2].legend(); axes[1, 2].grid(True)

    plt.tight_layout()
    plt.show()
