#!/usr/bin/env python3
"""
ReFind Synthetic Dataset Generator
Generates realistic LOST and FOUND report pairs with controlled noise and hidden ground truth (pair_id).
Used for evaluating Precision, Recall, Top-K Accuracy, and Query Latency scaling.
"""

import csv
import random
import argparse
from typing import List, Dict

CATEGORIES = {
    "phone": [
        "Samsung Galaxy S22", "iPhone 13 Pro", "OnePlus 11R", "Redmi Note 12",
        "Google Pixel 7", "iPhone 14", "Samsung Galaxy A54", "Vivo V27",
        "Realme 10 Pro", "Motorola Edge 40"
    ],
    "laptop": [
        "Dell XPS 13", "MacBook Air M2", "Lenovo ThinkPad X1", "HP Pavilion 15",
        "Asus ZenBook 14", "Acer Aspire 5", "MacBook Pro 16", "Dell Inspiron 14"
    ],
    "earphones": [
        "Boat Airdopes 141", "Apple AirPods Pro", "Sony WF-1000XM4", "Realme Buds Air 3",
        "JBL Wave 200", "OnePlus Nord Buds", "Noise Buds VS102"
    ],
    "bag": [
        "Wildcraft Backpack", "Skybags College Bag", "American Tourister Laptop Bag",
        "Nike Sports Duffel Bag", "Puma Casual Backpack", "Safari Trek Bag"
    ],
    "wallet": [
        "Wildhorn Leather Wallet", "Woodland Men Brown Wallet", "Tommy Hilfiger Bi-fold Wallet",
        "Titan Formal Wallet", "Hidesign Leather Purse"
    ],
    "keys": [
        "Hero Splendor Bike Key", "Honda Activa Key with keychain", "Room 304 Godrej Key",
        "Car Key with Honda Logo", "Apartment Main Door Key Bunch"
    ],
    "watch": [
        "Titan Fastrack Smartwatch", "Apple Watch SE", "Casio Vintage Digital Watch",
        "Noise ColorFit Pro", "Fossil Gen 6 Hybrid Smartwatch"
    ],
    "book": [
        "CLRS Introduction to Algorithms", "Operating Systems Principles Galvin",
        "Thomas Calculus 14th Edition", "Database System Concepts Silberschatz",
        "Computer Networks Tanenbaum"
    ],
    "card": [
        "State Bank of India Debit Card", "College Student ID Card", "Metro Smart Travel Card",
        "HDFC Credit Card", "Library Access Card"
    ],
    "umbrella": [
        "Fourseasons Folding Umbrella", "Sun Brand Large Black Umbrella", "Duck Handle Compact Umbrella"
    ],
    "water bottle": [
        "Milton Thermosteel Bottle 1L", "Cello Stainless Steel Flask 750ml",
        "Tupperware Sports Sipper", "Borosil Hydra Stainless Bottle"
    ],
    "usb drive": [
        "SanDisk Cruzer Blade 32GB", "Kingston DataTraveler 64GB",
        "HP Metal Pendrive 128GB", "SanDisk Ultra Dual Drive 64GB"
    ]
}

COLORS = ["black", "blue", "silver", "white", "grey", "red", "green", "brown", "gold", "matte black"]

CAMPUS_LOCATIONS = [
    "Library",
    "Computer Lab",
    "Physics Lab",
    "Lecture Hall A",
    "Lecture Hall B",
    "Main Canteen",
    "Nescafe Kiosk",
    "Sports Ground",
    "Badminton Court",
    "Hostel 1",
    "Hostel 2",
    "Main Gate Bus Stop",
    "Parking Lot"
]

DESCRIPTIONS = {
    "phone": [
        "cracked screen with transparent back cover",
        "black silicone case slightly scratched",
        "sticker of anime character on the back",
        "has tempered glass screen protector with small crack at corner",
        "white ring holder attached on back case"
    ],
    "laptop": [
        "sticker of GitHub octocat on top lid",
        "slight scratch near touchpad with black neoprene sleeve",
        "transparent plastic protective shell",
        "contains college sticker and developer stickers"
    ],
    "earphones": [
        "white charging case with red silicone cover",
        "left earbud has small scratch, black case",
        "inside black pouch with charging cable"
    ],
    "bag": [
        "blue college bag with water bottle in side mesh",
        "contains spiral notebooks and pencil pouch",
        "torn near right strap, yellow reflector stripe"
    ],
    "wallet": [
        "contains college id card and some cash",
        "black leather slightly worn out at corners",
        "brown bi-fold with photo inside transparent slot"
    ],
    "keys": [
        "attached with blue marvel keychain",
        "ring of three keys with red ribbon tag",
        "bike key with rubber grip and silver bottle opener"
    ],
    "watch": [
        "black strap with magnetic charger clip",
        "silver dial with leather strap slightly faded",
        "rubber sports band with tiny scratch on glass"
    ],
    "book": [
        "highlighted pages in chapter 4 with student name Sushil on first page",
        "hardcover edition with brown paper cover",
        "pencil notes written in margins"
    ],
    "card": [
        "in a plastic transparent badge holder with blue lanyard",
        "has passport size photo of student",
        "silver chip visible, signature on back"
    ],
    "umbrella": [
        "automatic push button handle, dark blue cloth",
        "foldable with small tear on umbrella sleeve",
        "wooden curved handle with wrist strap"
    ],
    "water bottle": [
        "metal dent on base with silver cap",
        "black matte finish with college club sticker",
        "blue sipper cap with measurement markings"
    ],
    "usb drive": [
        "small red slider cap attached to metal key ring",
        "black casing with 32gb engraved on front"
    ]
}

SYNONYM_REPLACEMENTS = {
    "phone": "mobile",
    "mobile": "phone",
    "backpack": "bag",
    "bag": "backpack",
    "wallet": "purse",
    "purse": "wallet",
    "glasses": "spectacles",
    "spectacles": "glasses",
    "earbuds": "earphones",
    "earphones": "earbuds",
    "usb": "pendrive",
    "pendrive": "usb"
}

def inject_typo(word: str) -> str:
    """Inject a realistic 1-character typo (deletion, transposition, or substitution)."""
    if len(word) <= 3:
        return word
    op = random.choice(["delete", "swap", "replace"])
    idx = random.randint(1, len(word) - 2)
    chars = list(word)
    if op == "delete":
        chars.pop(idx)
    elif op == "swap":
        chars[idx], chars[idx + 1] = chars[idx + 1], chars[idx]
    elif op == "replace":
        chars[idx] = random.choice("abcdefghijklmnopqrstuvwxyz")
    return "".join(chars)

def generate_dataset(num_records: int, match_ratio: float = 0.7, seed: int = 42) -> List[Dict]:
    random.seed(seed)
    records = []
    current_id = 1
    pair_id_counter = 1

    # Number of matched pairs (each produces 1 LOST and 1 FOUND)
    num_matched_pairs = int((num_records * match_ratio) / 2)
    num_unmatched = num_records - (num_matched_pairs * 2)

    # 1. Generate matched pairs
    for _ in range(num_matched_pairs):
        cat = random.choice(list(CATEGORIES.keys()))
        name = random.choice(CATEGORIES[cat])
        color = random.choice(COLORS)
        loc = random.choice(CAMPUS_LOCATIONS)
        desc_samples = DESCRIPTIONS.get(cat, ["item in good condition"])
        desc = random.choice(desc_samples)

        # Base LOST item
        lost_item = {
            "id": current_id,
            "type": "LOST",
            "category": cat,
            "name": name,
            "color": color,
            "location": loc,
            "description": desc,
            "pair_id": pair_id_counter
        }
        current_id += 1

        # FOUND item with realistic variations / noise
        # 25% chance of typo in name
        found_name = name
        if random.random() < 0.25:
            words = name.split()
            w_idx = random.randint(0, len(words) - 1)
            words[w_idx] = inject_typo(words[w_idx])
            found_name = " ".join(words)

        # 20% chance of synonym swap in description or name
        found_desc = desc
        for k, v in SYNONYM_REPLACEMENTS.items():
            if k in found_desc.lower() and random.random() < 0.4:
                found_desc = found_desc.replace(k, v)
                break

        # 15% chance different location (lost in library, turned in at canteen)
        found_loc = loc
        if random.random() < 0.15:
            found_loc = random.choice(CAMPUS_LOCATIONS)

        # 15% chance color is slightly varied or omitted
        found_color = color
        if random.random() < 0.15:
            found_color = random.choice(["dark", "matte " + color, ""])

        # 25% chance description is shorter / partial
        if random.random() < 0.25:
            desc_words = found_desc.split()
            if len(desc_words) > 3:
                found_desc = " ".join(desc_words[:max(2, len(desc_words) // 2)])

        found_item = {
            "id": current_id,
            "type": "FOUND",
            "category": cat,
            "name": found_name,
            "color": found_color,
            "location": found_loc,
            "description": found_desc,
            "pair_id": pair_id_counter
        }
        current_id += 1
        pair_id_counter += 1

        records.append(lost_item)
        records.append(found_item)

    # 2. Generate unmatched noise items (unpaired LOST or FOUND)
    for _ in range(num_unmatched):
        item_type = random.choice(["LOST", "FOUND"])
        cat = random.choice(list(CATEGORIES.keys()))
        name = random.choice(CATEGORIES[cat])
        color = random.choice(COLORS)
        loc = random.choice(CAMPUS_LOCATIONS)
        desc = random.choice(DESCRIPTIONS.get(cat, ["item found on campus"]))

        record = {
            "id": current_id,
            "type": item_type,
            "category": cat,
            "name": name,
            "color": color,
            "location": loc,
            "description": desc,
            "pair_id": -1 # Unmatched noise
        }
        current_id += 1
        records.append(record)

    # Shuffle so LOST and FOUND are distributed naturally
    random.shuffle(records)
    return records

def save_csv(records: List[Dict], filepath: str):
    fieldnames = ["id", "type", "category", "name", "color", "location", "description", "pair_id"]
    with open(filepath, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(records)
    print(f"Dataset saved to '{filepath}' with {len(records)} records.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate synthetic lost and found reports.")
    parser.add_argument("--count", type=int, default=1000, help="Number of records to generate (e.g. 1000, 10000)")
    parser.add_argument("--output", type=str, default="data/reports_1k.csv", help="Output CSV path")
    parser.add_argument("--seed", type=int, default=42, help="Random seed for reproducibility")
    args = parser.parse_args()

    data = generate_dataset(args.count, seed=args.seed)
    save_csv(data, args.output)
