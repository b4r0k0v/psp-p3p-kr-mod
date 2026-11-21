# 페르소나 3 포터블 PSP 한국어판 모드

PSP 한국어판 **UCKS45140 v1.00**에 전투 보상 배율과 페르소나 합체 스킬 수동 계승을 추가하는 모드다. 두 기능은 하나의 xdelta 패치로 함께 적용된다.

## 게임에서 사용하기

- **보상 배율** — 필드에서 `COMMAND → SYSTEM → 컨피그`를 열고 `SELECT`를 누른다. 위아래로 경험치 또는 돈을 선택하고 좌우로 `AUTO`, `×1.5`, `×2`, `×4`, `×8`을 바꾼다. `×`를 누르면 컨피그로 돌아간다. 돈 배율은 셔플 타임 코인 카드 보상에 적용된다. `AUTO`는 게임의 기본 보상 계산을 따르며 게임을 새로 실행하면 두 설정 모두 `AUTO`로 돌아간다.
- **스킬 수동 계승** — 합체 결과 미리보기에서 `○`를 눌러 선택을 시작한다. 방향키로 후보를 살펴보고 `○`로 고른다. 마지막 스킬을 고르면 합체 확인 대화로 넘어간다. 선택 중 `×`를 누르면 미리보기로 돌아간다. 선택 가능한 스킬과 계승 개수는 원본 합체 규칙을 따른다.

## xdelta 패치 적용

[통합 xdelta 패치](patches/p3p-kr-ucks45140-v1.00.xdelta)와 직접 보유한 한국어판 원본 ISO를 준비한다. 적용에는 `xdelta3`가 필요하다. 아래 명령은 WSL에서 저장소 루트를 기준으로 실행하며, `원본.iso`를 실제 파일 경로로 바꾼다.

1. `sha256sum "원본.iso"`를 실행하고 아래 표의 원본 ISO 값과 비교한다.
2. 패치를 적용해 새 ISO를 만든다. 이미 같은 이름의 파일이 있다면 새 출력 이름을 지정한다.

   ```bash
   xdelta3 -d -s "원본.iso" patches/p3p-kr-ucks45140-v1.00.xdelta "p3p-kr-patched.iso"
   ```

3. `sha256sum "p3p-kr-patched.iso"`를 실행하고 아래 표의 적용 후 값과 비교한다. 생성된 ISO를 Windows용 PPSSPP에서 열어 테스트용 세이브 사본으로 확인한다. 기존 게임으로 돌아가려면 원본 ISO를 연다.

| 파일 | SHA-256 |
| --- | --- |
| 원본 ISO | `09868e5bd1e659cb2b1b5ed761bb17a1ef821bc95fc7be42ec4e4d5f5964645d` |
| xdelta 패치 | `1cf82e435c6696d439fdef6aa420d3e35d4629c74d924d9a597e37c14a956db2` |
| 패치 적용 후 ISO | `da708cfea541c4ba971bbb4ef07cdbb51ae1b2044d7cf6f0edc74348cd292987` |

## 개발용 빌드

WSL2에서 `make`를 실행하면 두 기능을 포함한 페이로드가 빌드된다. ISO를 다시 만들려면 원본 ISO와 같은 게임의 PPSSPP 부팅 시 덤프한 복호화 `EBOOT.BIN`이 필요하다. 빌드에는 PSPDEV/PSPSDK와 Python 3를 사용한다.

```bash
make
python3 tools/build_reward_iso.py \
  --original "원본.iso" \
  --decrypted "복호화한-EBOOT.BIN" \
  --output build/p3p-kr-integrated.iso
```

게임 ISO, 실행 파일, 세이브는 로컬에 보관한다.

## 검증 현황

- PPSSPP에서 배율 메뉴 조작, 재실행 시 초기화, Beginner 난이도의 일부 경험치·코인 배율에 대한 표시와 실제 지급을 확인했다. 2랭크 코인 카드와 난이도별 보상, 최대치 등은 추가 검증이 필요하다.
- 2체 합체에서 선택한 스킬의 실제 계승을 확인했다. 3체 합체는 선택 및 확인 대화 진입까지 확인했으며 3체 합체의 최종 지급과 4~6체 특수 합체는 추가 검증이 필요하다.

## 저장소 구성

- `src/`에 배율 메뉴, 보상 지급 및 수동 계승 코드가 있다.
- `tools/build_reward_iso.py`는 패치된 ISO를 생성한다.
- `tools/test_battle_rewards.py`는 PPSSPP 원격 디버거로 보상 지급을 확인하는 개발용 스크립트다.
- 로컬 `docs/`와 `reverse/`에 기능 명세, 작업 계획, 분석 기록을 보관한다.
