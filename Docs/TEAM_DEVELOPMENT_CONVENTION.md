# 팀 개발 규약

## 명명 규칙 (Naming Convention)
- **클래스 및 파일 접두어:** 모든 C++ 클래스 및 스크립트 파일의 이름은 `Nova` 접두어를 사용한다. (예: `NovaUnit`, `NovaPlayerController`, `NovaAttributeSet`)
- **블루프린트 카테고리:** 모든 블루프린트 노출 변수 및 함수의 카테고리 대분류는 `Nova`를 사용한다. (예: `Category = "Nova|Unit"`, `Category = "Nova|GAS"`)
- **언리얼 엔진 표준 준수:** 그 외 클래스 타입 접두어(A, U, I 등)는 언리얼 엔진의 표준 명명 규칙을 따른다.

## 디버깅 및 로그 규약 (Debugging & Log Convention)
- **전용 매크로 사용:** `UE_LOG` 대신 프로젝트 전용 매크로인 `NOVA_LOG`, `NOVA_SCREEN`, `NOVA_CHECK`를 사용한다. 모든 매크로는 호출된 함수 이름과 줄 번호를 자동으로 포함한다.
- **NOVA_LOG(Verbosity, Format, ...):** 표준 출력창 로그용으로 사용한다. (예: `NOVA_LOG(Log, "Value: %d", Value)`)
- **NOVA_SCREEN(Verbosity, Format, ...):** 화면 및 출력창에 동시에 로그를 남길 때 사용한다. Verbosity에 따라 텍스트 색상이 자동으로 변경된다. (Log: Cyan, Warning: Yellow, Error: Red)
- **NOVA_CHECK(Condition, Format, ...):** 특정 조건이 실패했을 때 `Error` 로그를 남기기 위해 사용한다. `ensure`와 유사하게 동작하며, 실패 시 `CHECK FAILED` 메시지와 함께 상세 내용을 출력한다.

## 코드 리뷰 요구사항
- 모든 PR은 Repository 관리자의 승인 필요

## 이슈 관리 (Issue Management)
- **분류 체계:** 이슈는 제목의 접두어 대신 GitHub **Labels**를 사용하여 분류한다.
  - **Domain Labels:** `GAS`, `AI`, `Assembly`, `UI`, `Combat` 등 (작업 영역에 맞춰 중복 지정 권장)
- **템플릿 사용:** 모든 이슈는 GitHub에서 제공하는 탬플릿을 사용하여 생성해야 한다.
- **제목:** 간결하고 명확하게 작성하며, 레이블이 있으므로 별도의 접두어(`[Fix]`, `[Feat]` 등)는 강제하지 않는다.

## 커밋 규약
- Conventional Commits 형식 사용
- 각 커밋은 하나의 논리적 변경만 포함

## 브랜치 전략
- main: 개발/프로덕션 환경 코드.
- feature/*: 기능 개발 브랜치
- hotfix/*: 긴급 수정 브랜치

## Commit 양식 (Templete)
- 커밋 제목과 내용은 다음 양식을 따라야 함.
@../.gitmessage

## PR 양식 (Templete)
- Pull Request 제목과 내용은 다음 양식을 따라야 함.
@./WORK_LOG_TEMPLETE.md