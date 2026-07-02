# Nova Revolution

<img width="960" height="540" alt="스크린샷 2026-06-30 105958" src="https://github.com/user-attachments/assets/d4e5e799-b019-4bc8-9884-21a9e3238613" />

<p></p>

**Nova Revolution**은 고전 RTS 게임 **노바 1492**를 언리얼 엔진 5로 리메이크한 팀 포트폴리오 프로젝트입니다.
플레이어는 다양한 부품을 조합해 자신만의 커스텀 유닛을 제작하고, 이를 실시간으로 생산·지휘하여 적 기지를 파괴하는 것을 목표로 합니다.

---

## 목차

- [주요 특징](#주요-특징)
- [게임 플레이](#게임-플레이)
- [조작법](#조작법)
- [기술 스택](#기술-스택)
- [프로젝트 구조](#프로젝트-구조)
- [빌드 및 실행](#빌드-및-실행)
- [문서](#문서)
- [라이선스](#라이선스)

---

## 주요 특징

- **유닛 조립 시스템** — 다리 / 몸통 / 무기 / 악세사리 부품을 조합해 능력치가 합산되는 커스텀 유닛을 제작하고, 최대 10개까지 덱에 등록합니다.
- **GAS 기반 전투** — GameplayAbilitySystem, GameplayTag, GameplayCue를 활용한 데이터 주도적 전투 및 스킬 시스템.
- **RTS 전장의 안개** — 노바 1492 방식의 2단계 안개(가시 / 비가시) 시스템과 미니맵 연동.
- **AI 대전** — Behavior Tree / StateTree 기반 AI 플레이어와의 1대1 대전.
- **표준 + 고유 RTS 조작** — 부대 지정, 스마트 공격, 산개 등 RTS 표준 조작과 노바 1492 고유 조작 체계.
- **자원 및 인구 시스템** — 시간에 따라 자동 충전되는 와트/SP 자원과 와트 총합·개체 수 이중 제한.
- **성능 최적화** — NVIDIA DLSS 지원 및 오브젝트 풀링 서브시스템.

## 게임 플레이

- **장르:** 실시간 전략 (RTS)
- **플랫폼:** PC (Windows)
- **게임 모드:** 1vs1 PvE 대전
- **승리 조건:** 적의 기지를 파괴하면 승리, 아군의 기지가 파괴되면 패배

각 플레이어는 하나의 기지와 소량의 자원으로 시작합니다. 추가 건물은 건설할 수 없으며,
미리 조립해 둔 유닛 덱을 기지에서 즉시 생산하고 지휘하여 전황을 이끌어 갑니다.

자세한 게임 설계는 [게임 디자인 문서](Docs/GAME_DESIGN.md)를 참고하세요.

## 조작법

### RTS 표준 조작
| 조작 | 동작 |
| --- | --- |
| 좌클릭 / 드래그 | 유닛 선택 / 다중 선택 |
| 우클릭 | 이동·목표 지정 |
| `Ctrl` + `1`~`0` | 부대 지정 |
| `1`~`0` | 부대 선택 (연타 시 카메라 이동) |
| `A` → 좌클릭 | 스마트 공격 (이동 중 자동 교전) |
| `S` | 정지 (명령 취소) |
| `H` | 홀드 (제자리에서 사거리 내 적만 공격) |
| `P` | 순찰 |

### 노바 1492 고유 조작
| 조작 | 동작 |
| --- | --- |
| `Shift` + `1`~`0` | 해당 덱 유닛 즉시 생산 |
| `Alt` + `1`~`0` → 좌클릭 | 스킬 선택 후 목표 지정 발동 |
| `C` → 좌클릭 | 유닛 산개 |
| `L` | 유닛 완전 정지 |
| `B` | 기지 선택 (연타 시 카메라 이동) |

## 기술 스택

- **엔진:** Unreal Engine 5.7
- **언어:** C++ (핵심 로직) + Blueprint (UI / VFX / SFX / 애니메이션 / 데이터)
- **핵심 플러그인:** GameplayAbilities (GAS), StateTree / GameplayStateTree, EnhancedInput, UMG
- **그래픽:** NVIDIA DLSS SDK (DLSS / Streamline / Reflex / NIS)
- **협업:** Git (Git LFS 미사용)

## 프로젝트 구조

```
NovaRevolution/
├── Config/          # 프로젝트 설정 (ini)
├── Content/         # 에셋 (맵, 캐릭터, UI, VFX, 오디오 등)
├── Docs/            # 설계 및 개발 문서
└── Source/
    └── NovaRevolution/
        ├── Core/         # 게임 모드, 유닛, 기지, 자원, 안개, AI, 오브젝트 풀
        │   ├── AI/       # Behavior Tree 태스크·서비스, AI 컨트롤러
        │   ├── Animation/
        │   └── Production/
        ├── GAS/          # AttributeSet, GameplayTags, Abilities, GameplayCue
        ├── Input/        # EnhancedInput 설정
        ├── Lobby/        # 로비, 유닛 조립, 덱, 프리뷰
        ├── Player/       # 플레이어 폰·컨트롤러
        ├── UI/           # HUD, 미니맵, 선택, 자원, 유닛 정보 위젯
        ├── Variant_Strategy/
        └── Variant_TwinStick/
```

## 빌드 및 실행

### 요구 사항
- Unreal Engine **5.7**
- Visual Studio 2022 (Windows용 C++ 게임 개발 워크로드)
- [NVIDIA DLSS 플러그인](https://www.unrealengine.com/marketplace/en-US/product/nvidia-dlss)

### 빌드 절차
1. 저장소를 클론합니다.
   ```bash
   git clone https://github.com/ThunderVolt45/Nova-Revolution.git
   ```
2. `NovaRevolution.uproject`를 우클릭하여 **Generate Visual Studio project files**를 실행합니다.
3. `NovaRevolution.sln`을 Visual Studio에서 열고 **Development Editor / Win64** 구성으로 빌드합니다.
4. 빌드가 완료되면 `NovaRevolution.uproject`를 실행합니다.

## 문서

| 문서 | 설명 |
| --- | --- |
| [GAME_DESIGN.md](Docs/GAME_DESIGN.md) | 게임 디자인 설계서 |
| [TEAM_DEVELOPMENT_CONVENTION.md](Docs/TEAM_DEVELOPMENT_CONVENTION.md) | 팀 개발 규약 |
| [PLAYER_GAS_SYSTEM.md](Docs/PLAYER_GAS_SYSTEM.md) | 플레이어 GAS 시스템 |
| [UNIT_GAS_SYSTEM.md](Docs/UNIT_GAS_SYSTEM.md) | 유닛 GAS 시스템 |
| [UNIT_COMBAT_GAS_SYSTEM.md](Docs/UNIT_COMBAT_GAS_SYSTEM.md) | 유닛 전투 GAS 시스템 |
| [UNIT_AI_ARCHITECTURE.md](Docs/UNIT_AI_ARCHITECTURE.md) | 유닛 AI 아키텍처 |
| [AI_PLAYER_SYSTEM.md](Docs/AI_PLAYER_SYSTEM.md) | AI 플레이어 시스템 |
| [OBJECT_POOL_SYSTEM.md](Docs/OBJECT_POOL_SYSTEM.md) | 오브젝트 풀 시스템 |

## 라이선스

이 프로젝트는 [MIT License](LICENSE) 하에 배포됩니다.
