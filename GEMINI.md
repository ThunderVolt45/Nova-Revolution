# Nova Revolution 프로젝트

## 프로젝트 요약
- **프로젝트 명:** Nova Revolution
- **장르:** RTS
- **목표:** 언리얼 엔진 개발자 포트폴리오로서의 RTS 게임 '노바 1492' 리메이크 (노바 1492 모작)
- **엔진 버전:** 언리얼 엔진 5.7.x
  - **추가 플러그인:** NVIDIA DLSS SDK
- **플랫폼:** Windows

## 개발 규약

### 프로젝트 관리
- **Git 기반 팀 협업:** 3인 규모의 팀 프로젝트로 진행한다.
  - **전략:** `.uasset` 등 바이너리 파일들의 충돌을 최소화하기 위해 가능한 한 Git으로 관리 가능한 C++ 코드 위주로 개발한다.
- **Git LFS 배제:** 비용 문제로 인해 비활성화
  - **전략:** `.uasset`, `.umap` 등의 파일들의 크기를 100MB 이하로 제한한다.
  - **최적화:** 대형 소스 파일의 커밋을 제한한다. (High-poly FBXs, 4K+ 무압축 텍스쳐 등)

### 아키텍처 설계
- **데이터 주도 개발:** 프로젝트는 GameplayAbilitySystem (GAS), GameplayTag, GameplayCue, Data asset, Data Tables 등을 활용해 데이터 주도적으로 개발되어야 한다.
- **명시적 실패:** 프로젝트 내 모든 코드는 오류가 발생할 경우, 디버그 메시지 등으로 출력하거나 CastCheck<>(), check(), ensure() 등의 기능을 사용해 코드를 명시적으로 실패시켜야 한다.

### 코드 스타일
- **C++:** 핵심 로직, UI 제어 로직 등 C++로 구현 가능한 로직들은 가능한 한 C++ 클래스로 구현되어야 한다.
- **Blueprints:** UI (Widget Blueprints), SFX/VFX (GameplayCue), 애니메이션 (Animation Blueprints), 데이터 테이블, 기타 데이터 수정 등 블루프린트 상에서 작업되어야 하는 것들에 한해서 사용되어야 한다.
- **명명 규칙:** 언리얼 엔진 표준 명명 규칙을 따라간다. (BP_, M_, T_, S_, etc.). 
- **폴더 구조:** 언리얼 엔진 표준 폴더 규조를 따라간다.
- **코드 주석:** 코드 내 주석 작성은 고유 명사를 제외한 것들은 모두 한글로 작성되어야 한다.

## 추가 규약
@./Docs/GAME_DESIGN.md
@./Docs/TEAM_DEVELOPMENT_CONVENTION.md