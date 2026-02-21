# Emotion Path Optimizer (Dijkstra-Based Emotional Guidance System)

## Overview
This project is a novel application of Dijkstra’s Shortest Path Algorithm
to emotional state modeling.

Emotions are represented as nodes in a graph.
Transitions between emotions are weighted edges.
The algorithm computes the smoothest path from distress to positive states.

## Features
- Graph-based emotional modeling
- Dijkstra shortest path implementation in C
- Emotional realism (blocked unrealistic transitions)
- Predefined coping strategies
- User-defined tips & actions
- Persistent save/load (emotion_data.txt)

## How It Works
Each emotional transition has a difficulty weight.
The system adjusts weights using:
- Presence of coping tips
- Defined transition actions
- Internal emotional bias

Dijkstra’s algorithm finds the lowest-cost emotional path.

## Compile & Run

```bash
gcc -std=c99 -Wall -O2 emotion_final.c -o emo_tool
./emo_tool
