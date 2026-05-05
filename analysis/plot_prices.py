import csv
import matplotlib.pyplot as plt
import sys

def plot_prices(infile, outfile):
    times = []
    prices = []

    with open(infile, newline="") as f:
        reader = csv.DictReader(f)

        for row in reader:
            timestamp = int(row["timestamp"])
            price = int(row["price"])

            if price != 0:
                times.append(timestamp / 1_000_000_000 / 3600)
                prices.append(price / 10_000)

    plt.figure()
    plt.plot(times, prices)
    plt.xlabel("Hours Since Midnight")
    plt.ylabel("Price")
    plt.title("Best Bid Price")
    plt.grid(True)

    plt.tight_layout()
    plt.savefig(outfile, dpi=300)
    plt.show()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("python plot_prices.py [prices csv] [path to output file]")
        sys.exit(1)

    infile = sys.argv[1];
    outfile = sys.argv[2];

    plot_prices(infile, outfile)
