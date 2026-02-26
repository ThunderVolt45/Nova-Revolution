# Nova Revolution - Project Context

## Project Overview
- **Goal:** Remake of 'Nova 1492' (노바 1492 모작)
- **Engine Version:** Unreal Engine 5.7.x
- **Team Size:** 3 Members
- **Platform:** PC (Steam/GitHub)

## Core Mandates & Constraints
- **Git LFS:** Disabled (Due to cost constraints).
  - **Strategy:** Keep individual `.uasset` and `.umap` files under 100MB.
  - **Optimization:** Regularly clean up unused assets and avoid committing large raw source files (High-poly FBXs, 4K+ Uncompressed Textures).
- **Architecture:** Nova 1492 style Unit Assembly System.
  - Legs (다리) + Body (몸통) + Arms (무기/팔).
  - Data-driven approach using Data Assets or Data Tables.

## Development Guidelines
- **C++:** Primary logic, assembly systems, network foundation.
- **Blueprints:** UI, visual effects, animation logic, and data tweaking.
- **Naming Convention:** Follow standard UE naming conventions (BP_, M_, T_, S_, etc.).
