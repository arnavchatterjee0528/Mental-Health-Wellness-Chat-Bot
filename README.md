# 🧠 Mental Health Wellness Chat Bot
### Dijkstra-Based Emotional State Optimization System (C Project)

---

## 📌 Overview

This project applies **Dijkstra’s Shortest Path Algorithm** to emotional state modeling.

Emotions are represented as nodes in a weighted graph, and transitions between emotions are represented as edges with difficulty values. The system computes the smoothest emotional path from a distress state to a positive state such as calm, hopeful, or happy.

This project demonstrates how classical graph algorithms can be extended beyond traditional routing problems into abstract and assistive computational systems.

---

## 🚀 Features

- Graph-based emotional modeling  
- Dijkstra’s shortest path algorithm implementation in C  
- Realistic emotional transition constraints  
- Predefined coping strategies  
- User-defined emotional tips & actions  
- Persistent save/load system  

---

## 💻 How to Run

### 1️⃣ Clone the repository
git clone https://github.com/arnavchatterjee0528/Mental-Health-Wellness-Chat-Bot.git
cd Mental-Health-Wellness-Chat-Bot

### 2️⃣ Compile the program
gcc emotion_final.c -o emo_tool
### 3️⃣ Run the program

Windows:emo_tool
Mac/Linux:./emo_tool
---

## 📂 Project Structure
emotion_final.c
README.md
.gitignore
emotion_data.txt (auto-generated after running)


---

## 🧠 How It Works

1. Emotions are treated like locations on a map.
2. Each transition has a difficulty value.
3. Coping strategies reduce transition difficulty.
4. Dijkstra’s algorithm finds the easiest emotional route.
5. Unrealistic transitions are restricted for realism.

---

## 👨‍💻 Author

Arnav Chatterjee  
B.Tech Computer Science  
2026
