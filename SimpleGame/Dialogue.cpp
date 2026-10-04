#include "stdafx.h"
#include "Dialogue.h"
#include "Progress.h"

namespace
{
	using Dialogue::Line;
	using Dialogue::Choice;
	using Dialogue::Script;
	using Dialogue::Style;
	using Dialogue::kHero;
	using Dialogue::kEnd;

	template <typename T, size_t N>
	constexpr int CountOf(const T(&)[N])
	{
		return (int)N;
	}

	// ---- The priests ----
	// Each side's priest speaks of the one who made his own side, and of the
	// other side as no one's making at all - the same words, mirrored.

	const Line kPriestLight[] =
	{
		{ "사제", "제단은 누구에게나 열려 있소. 지친 자든, 길 잃은 자든.", Style::Speech },
		{ "사제", "빛을 지으신 분께서 그대의 힘이 어디로 기우는지 늘 보고 계시오.", Style::Speech },
	};
	const Choice kPriestLightChoices[] =
	{
		{ "신에 대해 묻는다", Story::NoFlag, Dialogue::PriestOnGodLight },
		{ "아무것도 아니다", Story::NoFlag, kEnd },
	};

	const Line kPriestOnGodLight[] =
	{
		{ kHero, "빛을 지은 신이라면, 어둠은 누가 지었소?", Style::Speech },
		{ "사제", "어둠은 빛이 비추지 못한 자리일 뿐이오. 지은 이가 따로 있을 리 없지.", Style::Speech },
		{ "사제", "의심은 잿빛으로 가는 첫걸음이오. 마음을 단단히 하시오.", Style::Speech },
	};

	// Once he is out of the god's reach, the same question no longer reaches
	// the priest at all.
	const Line kPriestErrorLight[] =
	{
		{ kHero, "빛을 지은 신이라면, 어둠은 누가 지었소?", Style::Speech },
		{ "사제", "빛을 지으신 분께서 그대의 힘이 어디로 기우는지 늘 보고 계시오.", Style::Error },
		{ kHero, "그걸 물은 게 아니오.", Style::Speech },
		{ "사제", "빛을 지으신 분께서 그대의 힘이 어디로 기우는지 늘 보고 계시오.", Style::Error },
		{ "사제", "……어째서 같은 말을 몇 번이고 하게 만드는 거요! 썩 물러가시오!", Style::Speech },
		{ kHero, "방금 그건…… 내 말이 들리지 않은 건가.", Style::Thought },
	};

	const Line kPriestDark[] =
	{
		{ "사제", "들어오시오. 제단 앞에서는 칼을 거두는 것이 이곳의 법이오.", Style::Speech },
		{ "사제", "어둠을 지으신 분께서 그대의 힘이 어디로 기우는지 늘 보고 계시오.", Style::Speech },
	};
	const Choice kPriestDarkChoices[] =
	{
		{ "신에 대해 묻는다", Story::NoFlag, Dialogue::PriestOnGodDark },
		{ "아무것도 아니다", Story::NoFlag, kEnd },
	};

	const Line kPriestOnGodDark[] =
	{
		{ kHero, "어둠을 지은 신이라면, 빛은 누가 지었소?", Style::Speech },
		{ "사제", "빛은 어둠이 미처 덮지 못한 상처일 뿐이오. 지은 이가 따로 있을 리 없지.", Style::Speech },
		{ "사제", "그런 물음은 잿빛의 입에서나 나오는 것이오. 조심하시오.", Style::Speech },
	};

	const Line kPriestErrorDark[] =
	{
		{ kHero, "어둠을 지은 신이라면, 빛은 누가 지었소?", Style::Speech },
		{ "사제", "어둠을 지으신 분께서 그대의 힘이 어디로 기우는지 늘 보고 계시오.", Style::Error },
		{ kHero, "그걸 물은 게 아니오.", Style::Speech },
		{ "사제", "어둠을 지으신 분께서 그대의 힘이 어디로 기우는지 늘 보고 계시오.", Style::Error },
		{ "사제", "……같은 말을 몇 번이나 하게 할 셈이오! 당장 나가시오!", Style::Speech },
		{ kHero, "방금 그건…… 내 말이 들리지 않은 건가.", Style::Thought },
	};

	// ---- The kings, as told on the road ----
	// They point him at the duel without a word about where it leads.

	const Line kRumorLucius[] =
	{
		{ "마을 사람", "루멘에 갈 생각이오? 왕궁 앞에 서면 루키우스 폐하께서 직접 상대해 주신다더군.", Style::Speech },
		{ "마을 사람", "폐하를 꺾고 인정받은 자는 빛의 축복을 받는다던데…… 경계 냄새 나는 자가 해낼 리 없겠지만.", Style::Speech },
	};

	const Line kRecognizedLucius[] =
	{
		{ "마을 사람", "들었소. 폐하께서 경계에서 온 자를 인정하셨다고.", Style::Speech },
		{ "마을 사람", "세상이 어찌 되려는지. 그래도 폐하의 뜻이라면 따라야지.", Style::Speech },
	};

	const Line kRumorNoctis[] =
	{
		{ "마을 사람", "테네브라에로 간다고? 녹티스 왕은 찾아온 자를 마다하지 않아. 살려 보낼지는 그분 마음이지만.", Style::Speech },
		{ "마을 사람", "그 왕에게 인정받으면 어둠의 축복을 받는다더군. 계산에 넣어 둘 만한 자라면 말이야.", Style::Speech },
	};

	const Line kRecognizedNoctis[] =
	{
		{ "마을 사람", "녹티스 왕이 너를 인정했다고? 그 왕이 손해 보는 계산을 했을 리 없는데.", Style::Speech },
		{ "마을 사람", "뭐, 이제 너도 그분 장부 어딘가에 적혀 있겠군.", Style::Speech },
	};

	// ---- The capitals ----

	const Line kCitizenLumen[] =
	{
		{ "루멘 시민", "폐하께서는 자비로우시오. 쓰러진 자의 목숨까지 거두지는 않으시지.", Style::Speech },
		{ "루멘 시민", "……경계인에게도 그러실지는 모르겠지만.", Style::Speech },
	};

	const Line kCitizenTenebrae[] =
	{
		{ "테네브라에 시민", "왕궁 앞 카펫은 밟지 않는 게 좋아. 밟는 순간 폐하의 셈이 시작되거든.", Style::Speech },
	};

	// ---- The hidden village ----

	const Line kLiminaElder[] =
	{
		{ "촌장", "@, 또 숲 가장자리까지 나갔었느냐. 네 힘이 커질수록 우리 냄새도 멀리 퍼진다.", Style::Speech },
		{ "촌장", "우리는 숨어 산 덕에 살아남았다. 그걸 잊지 마라.", Style::Speech },
	};

	const Line kLiminaChild[] =
	{
		{ "아이", "있잖아, 아까 숲에서 뭔가 빛나는 거 봤어! 그거 또 네가 한 거지?", Style::Speech },
		{ "아이", "나도 크면 그렇게 할 수 있어? 아, 촌장님한테는 비밀이야!", Style::Speech },
	};

	// ---- The prologue ----

	const Line kPrologueOpening[] =
	{
		{ nullptr, "빛과 어둠이 서로를 죽이며 살아가는 세상. 그 경계에서 태어난 자들은 어느 쪽에서도 사람으로 여겨지지 않았다.", Style::Speech },
		{ nullptr, "잿빛이 세상을 물들인 뒤로는 더했다. 두 나라는 경계에서 태어난 자를 보는 즉시 죽이라는 법을 세웠다.", Style::Speech },
		{ nullptr, "그래서 그들은 숨었다. 숲 깊은 곳, 어느 지도에도 없는 마을 리미나에.", Style::Speech },
		{ nullptr, "숲에 버려져 있던 아이 하나가 그 마을에서 자랐다. 마을 사람들이 지어 준 이름은, @.", Style::Speech },
		{ kHero, "오늘따라 숲이 너무 조용하다.", Style::Thought },
	};

	const Line kPrologueRaid[] =
	{
		{ nullptr, "숲 쪽에서, 잿빛이 번져 왔다.", Style::Speech },
		{ "촌장", "잿빛의 것들이다……! 모두 숨어라! 아이들부터!", Style::Speech },
		{ "아이", "@! 저기, 저것들이……!", Style::Speech },
		{ kHero, "들켰다. 내 힘 때문에.", Style::Thought },
	};

	const Line kPrologueAftermath[] =
	{
		{ kHero, "……아무도 남지 않았다.", Style::Thought },
		{ kHero, "촌장님도, 그 아이도. 잿빛에 삼켜져 흔적조차 없다.", Style::Thought },
		{ kHero, "잿빛이 온 쪽은 숲 너머다. 그 끝에 무엇이 있든, 전부 없애 주겠다.", Style::Thought },
		{ nullptr, "마을 한가운데의 돌기둥이 희미하게 빛나기 시작했다. 이제 떠날 때다.", Style::Speech },
	};

	// ---- Kinis ----

	const Line kKinisFirst[] =
	{
		{ nullptr, "잿빛이 가장 짙게 내려앉은 자리에, 누군가 서 있었다.", Style::Speech },
		{ "잿빛의 존재", "경계의 냄새가 나는군. 리미나에서 왔나.", Style::Speech },
		{ kHero, "……네가 이 잿빛을 퍼뜨린 자인가.", Style::Speech },
		{ "잿빛의 존재", "그래. 그렇게 불려도 할 말은 없지.", Style::Speech },
		{ "키니스", "키니스. 잿빛이 되기 전에는 다른 이름으로 불렸다. 가장 어두운 빛, 가장 밝은 어둠.", Style::Speech },
		{ kHero, "그 이명은…… 경계의 옛이야기에 나오는 이름이다.", Style::Thought },
		{ "키니스", "나도 너처럼 경계에서 태어났다. 너처럼, 너무 강했고.", Style::Speech },
		{ "키니스", "그래서 두 왕이 손을 잡았다. 평생 서로를 죽이던 빛의 왕과 어둠의 왕이, 나 하나를 지우려고.", Style::Speech },
		{ "키니스", "그들은 내 곁의 사람을 먼저 데려갔다. 그다음은…… 보는 대로다.", Style::Speech },
		{ kHero, "빛과 어둠이 결탁했다고?", Style::Speech },
		{ "키니스", "믿지 않아도 좋다. 언젠가 그들에게 직접 물어보면 되겠지. 대답을 들을 수 있다면 말이다.", Style::Speech },
		{ "키니스", "잿빛의 한가운데, 키네라에서 기다리마. 그때 네가 무엇이 되어 있을지 궁금하군.", Style::Speech },
		{ nullptr, "잿빛이 흩어지듯, 그의 모습도 사라졌다.", Style::Speech },
	};

	// ---- The kings, recognized ----
	// Asked about the collusion, each gives back the last thing it said, as
	// if the question had never been spoken.

	const Line kKingLucius[] =
	{
		{ "루키우스", "그대는 이미 인정받았다. 빛의 축복이 그대와 함께하기를.", Style::Speech },
	};

	const Line kKingLuciusAsk[] =
	{
		{ "루키우스", "그대는 이미 인정받았다. 빛의 축복이 그대와 함께하기를.", Style::Speech },
	};
	const Choice kKingLuciusAskChoices[] =
	{
		{ "결탁에 대해 묻는다", Story::AskedLucius, Dialogue::KingLuciusError },
		{ "아무것도 아니다", Story::NoFlag, kEnd },
	};

	const Line kKingLuciusError[] =
	{
		{ kHero, "키니스에게 들었소. 당신과 녹티스가 손을 잡고 그를 없애려 했다고.", Style::Speech },
		{ "루키우스", "그대는 이미 인정받았다. 빛의 축복이 그대와 함께하기를.", Style::Error },
		{ kHero, "결탁에 대해 묻고 있소.", Style::Speech },
		{ "루키우스", "그대는 이미 인정받았다. 빛의 축복이 그대와 함께하기를.", Style::Error },
		{ "루키우스", "……같은 말을 되풀이하게 하지 마라! 물러가라!", Style::Speech },
		{ kHero, "못 알아들은 게 아니다. 내 말이…… 그에게 닿지 않았다.", Style::Thought },
	};

	const Line kKingNoctis[] =
	{
		{ "녹티스", "인정은 이미 했다. 더 볼일이 있나? 내 셈에 없는 일이라면 사양하지.", Style::Speech },
	};

	const Line kKingNoctisAsk[] =
	{
		{ "녹티스", "인정은 이미 했다. 더 볼일이 있나? 내 셈에 없는 일이라면 사양하지.", Style::Speech },
	};
	const Choice kKingNoctisAskChoices[] =
	{
		{ "결탁에 대해 묻는다", Story::AskedNoctis, Dialogue::KingNoctisError },
		{ "아무것도 아니다", Story::NoFlag, kEnd },
	};

	const Line kKingNoctisError[] =
	{
		{ kHero, "키니스에게 들었소. 당신과 루키우스가 손을 잡고 그를 없애려 했다고.", Style::Speech },
		{ "녹티스", "인정은 이미 했다. 더 볼일이 있나? 내 셈에 없는 일이라면 사양하지.", Style::Error },
		{ kHero, "결탁에 대해 묻고 있소.", Style::Speech },
		{ "녹티스", "인정은 이미 했다. 더 볼일이 있나? 내 셈에 없는 일이라면 사양하지.", Style::Error },
		{ "녹티스", "……같은 말을 하게 만들지 마라. 다음엔 칼로 대답하겠다.", Style::Speech },
		{ kHero, "그 셈 빠른 왕이, 질문 하나를 듣지 못했다.", Style::Thought },
	};

	// ---- The rescues ----
	// The same prayer from both sides, to the same one, word for word.

	const Line kRescueLight[] =
	{
		{ "피데스 주민", "살았다…… 당신이 아니었으면 나도 잿빛이 됐을 거요.", Style::Speech },
		{ "피데스 주민", "경계의 피라도 상관없소. 오늘만큼은.", Style::Speech },
		{ nullptr, "그는 무너진 제단 앞에 무릎을 꿇었다. 낮은 목소리가 흘러나왔다.", Style::Speech },
		{ "피데스 주민", "판을 지키시는 분이시여, 오늘도 우리가 설 자리를 남겨 주소서. 맞설 이를 잃지 않게 하소서.", Style::Speech },
		{ kHero, "빛을 지은 신에게 드리는 기도치고는…… 이상한 말이다.", Style::Thought },
	};

	const Line kRescueDark[] =
	{
		{ "레퀴엠 주민", "……빚졌군. 경계인에게 빚을 지다니, 세상 참.", Style::Speech },
		{ nullptr, "그는 무너진 제단 앞에 무릎을 꿇었다. 낮은 목소리가 흘러나왔다.", Style::Speech },
		{ "레퀴엠 주민", "판을 지키시는 분이시여, 오늘도 우리가 설 자리를 남겨 주소서. 맞설 이를 잃지 않게 하소서.", Style::Speech },
		{ kHero, "어둠을 지은 신에게 드리는 기도치고는…… 이상한 말이다.", Style::Thought },
	};

	const Line kSurvivorLight[] =
	{
		{ "피데스 주민", "판을 지키시는 분께서 당신을 보내셨나 보오.", Style::Speech },
	};

	const Line kSurvivorDark[] =
	{
		{ "레퀴엠 주민", "빚은 갚는다. 언젠가는.", Style::Speech },
	};

	const Line kEavesdropChosen[] =
	{
		{ kHero, "같은 기도다. 한 글자도 다르지 않다.", Style::Thought },
		{ kHero, "빛을 지은 신도, 어둠을 지은 신도 아니었다. 양쪽 모두 같은 '판을 지키시는 분'에게 빌고 있었다.", Style::Thought },
		{ kHero, "두 나라가 믿는 신은…… 하나다.", Style::Thought },
		{ kHero, "왕들이 내 물음을 듣지 못했던 것도, 그 신 때문인가.", Style::Thought },
		{ nullptr, "머릿속을 누르던 무언가가 걷혀 나갔다. 처음으로, 세상이 똑바로 보였다.", Style::Speech },
		{ nullptr, "이명을 얻었다  ·  신에게 선택받은 자", Style::Speech },
	};

	const Line kEavesdropClouded[] =
	{
		{ kHero, "같은 기도다. 한 글자도 다르지 않다.", Style::Thought },
		{ kHero, "두 나라가 같은 것을 믿는다면…… 왕들이 내 말을 듣지 못한 것도……", Style::Thought },
		{ kHero, "……생각이 이어지지 않는다. 무언가가 머릿속을 짓누른다.", Style::Thought },
		{ kHero, "더 강해져야 한다. 이 무게를 밀어낼 수 있을 만큼.", Style::Thought },
	};

	const Line kEavesdropUnaware[] =
	{
		{ kHero, "같은 기도다. 빛이든 어둠이든, 오래된 기도는 다 비슷한 모양이지.", Style::Thought },
		{ kHero, "……그런가 보다.", Style::Thought },
	};

	const Line kChosenLater[] =
	{
		{ kHero, "그 기도가 다시 떠오른다. 판을 지키시는 분이시여……", Style::Thought },
		{ kHero, "이제는 생각이 끊기지 않는다. 두 나라가 믿는 신은 하나다.", Style::Thought },
		{ kHero, "왕들이 내 물음을 듣지 못했던 것도, 그 신 때문이다.", Style::Thought },
		{ nullptr, "머릿속을 누르던 무언가가 걷혀 나갔다. 처음으로, 세상이 똑바로 보였다.", Style::Speech },
		{ nullptr, "이명을 얻었다  ·  신에게 선택받은 자", Style::Speech },
	};

	// ---- The front ----

	const Line kRenewedWar[] =
	{
		{ nullptr, "전선 한가운데, 낯익은 두 사람이 서로에게 칼을 겨누고 있었다.", Style::Speech },
		{ "피데스 주민", "어둠의 것들이 또 국경을 넘었다. 이번엔 물러서지 않겠다.", Style::Speech },
		{ "레퀴엠 주민", "빛의 개가 짖는군. 잿빛에서 건져진 목숨, 여기서 버려라.", Style::Speech },
		{ kHero, "그만두시오! 둘 다 같은 기도를 올리던 사람들이잖소!", Style::Speech },
		{ "피데스 주민", "어둠의 것들이 또 국경을 넘었다. 이번엔 물러서지 않겠다.", Style::Error },
		{ "레퀴엠 주민", "빛의 개가 짖는군. 잿빛에서 건져진 목숨, 여기서 버려라.", Style::Error },
		{ kHero, "들리지 않는다. 내가 구한 두 사람이, 아무 이유도 없이 서로를 미워한다.", Style::Thought },
		{ kHero, "이 증오는…… 그들의 것이 아니다.", Style::Thought },
		{ nullptr, "두 사람은 칼을 맞댄 채 잿빛 너머로 사라졌다.", Style::Speech },
	};

	// ---- Kinis, at the end ----

	const Line kKinisOffer[] =
	{
		{ nullptr, "잿빛의 한가운데. 키니스가 기다리고 있었다.", Style::Speech },
		{ "키니스", "왔군. 생각보다 오래 걸렸어.", Style::Speech },
		{ "키니스", "잿빛을 걷어 내며 왔겠지. 덕분에 나는 이만큼 약해졌다.", Style::Speech },
		{ "키니스", "이제 알겠지. 빛과 어둠은 서로를 죽이지 않고는 살 수 없다. 내가 사라져도 전쟁은 다시 시작된다.", Style::Speech },
		{ "키니스", "그러니 함께 가자. 너와 내가 잿빛이 되면 빛도 어둠도 남지 않는다. 싸울 이유도.", Style::Speech },
	};
	const Choice kKinisOfferChoices[] =
	{
		{ "제안을 받아들인다", Story::AcceptAshOffer, Dialogue::KinisAccept },
		{ "거절한다", Story::NoFlag, Dialogue::KinisRefuse },
	};

	const Line kKinisAccept[] =
	{
		{ "키니스", "……그래. 경계에서 태어난 자들에게 어울리는 끝이지.", Style::Speech },
		{ nullptr, "잿빛이 두 사람을 감쌌다.", Style::Speech },
	};

	const Line kKinisRefuse[] =
	{
		{ "키니스", "그럴 줄 알았다. 그렇다면 네 힘으로 증명해 봐라.", Style::Speech },
	};

	const Line kKinisSilent[] =
	{
		{ nullptr, "잿빛의 한가운데. 키니스가 서 있었다. 그런데, 무언가 달랐다.", Style::Speech },
		{ kHero, "키니스. 왕들의 결탁은 두 나라가 믿는 하나의 신 때문이었소. 그걸 말하러 왔소.", Style::Speech },
		{ "키니스", "왔군. 생각보다 오래 걸렸어.", Style::Error },
		{ kHero, "내 말이 들리오?", Style::Speech },
		{ "키니스", "왔군. 생각보다 오래 걸렸어.", Style::Error },
		{ kHero, "왕들과 같다. 잿빛이 걷히는 동안 그는 약해졌고…… 신의 손이 그에게 닿았다.", Style::Thought },
		{ kHero, "내가 그의 힘을 빼앗았다. 그래서 이제, 말이 통하지 않는다.", Style::Thought },
		{ nullptr, "키니스가 칼을 들었다.", Style::Speech },
	};

	// Heard through the god's interference: only the words that matter get
	// through, and enough of them to piece it together.
	const Line kKinisLastBroken[] =
	{
		{ nullptr, "쓰러진 키니스가 무언가를 말하려 했다. 목소리가 이상하게 끊겨 들렸다.", Style::Speech },
		{ "키니스", "들어라…… [이 세상은 처음부터] {신}[이 벌여 놓은] {놀이}[였다.]", Style::Broken },
		{ "키니스", "[빛과 어둠은 서로를 죽이도록] {본능}[을 새겨 받았다. 그것이] {놀이}[의 규칙이다.]", Style::Broken },
		{ "키니스", "[두 왕의] {결탁}[도…… 내가 판을 흔들었기에,] {신}[의 뜻대로 움직인 것이다.]", Style::Broken },
		{ "키니스", "[부디…… 이 판을] {끝내}[라.]", Style::Broken },
		{ nullptr, "그의 몸이 잿빛 먼지가 되어 흩어졌다.", Style::Speech },
		{ kHero, "신, 놀이, 본능, 결탁…… 끊긴 말 사이로 그 단어들만 남았다.", Style::Thought },
		{ kHero, "빛과 어둠이 서로를 미워하는 것이, 누군가 그렇게 만들어 둔 것이라면……", Style::Thought },
		{ kHero, "……모르겠다. 지금 아는 것은, 잿빛이 끝났다는 것뿐이다.", Style::Thought },
	};

	const Line kKinisLastClear[] =
	{
		{ nullptr, "쓰러진 키니스의 눈에서, 낯선 빛이 걷혔다.", Style::Speech },
		{ "키니스", "……이제야 네 목소리가 들리는군. 늦었지만.", Style::Speech },
		{ "키니스", "들어라. 이 세상은 처음부터 신이 벌여 놓은 놀이였다. 빛과 어둠은 서로를 죽이도록 본능을 새겨 받았다. 그것이 놀이의 규칙이다.", Style::Speech },
		{ "키니스", "두 왕의 결탁도, 내가 판을 흔들었기에 신의 뜻대로 움직인 것이다. 잿빛은…… 신도 몰랐던 어긋남이었지.", Style::Speech },
		{ "키니스", "내가 남긴 잿빛을 받아들여라. 빛과 어둠, 그리고 잿빛. 셋을 쥔 자라면 판 위에 머물지 않을 수 있다.", Style::Speech },
		{ "키니스", "받아들이지 않으면 전쟁은 다시 시작되고…… 판을 벗어난 너를, 신은 그냥 두지 않을 것이다.", Style::Speech },
		{ nullptr, "그의 몸이 잿빛 먼지가 되어 흩어졌다. 남은 잿빛이 @의 발치에 고였다.", Style::Speech },
	};
	const Choice kKinisLastClearChoices[] =
	{
		{ "잿빛을 받아들인다", Story::AcceptAsh, kEnd },
		{ "받아들이지 않는다", Story::NoFlag, kEnd },
	};

	// ---- The god ----

	const Line kGodMeeting[] =
	{
		{ nullptr, "하늘에 구멍이 나 있었다. 빛도 어둠도 아닌, 아무것도 없는 자리.", Style::Speech },
		{ nullptr, "그 가장자리를 두 개의 손이 천천히 돌고 있었다.", Style::Speech },
		{ "「   」", "돌아왔구나, 나의 말아.", Style::Speech },
		{ kHero, "……말?", Style::Speech },
		{ "「   」", "숲에 너를 놓은 것은 나다. 잿빛을 거두라고. 판을 되돌리라고.", Style::Speech },
		{ "「   」", "쓰러뜨린 자의 힘을 거두는 그 손도, 끝없이 커지는 그 그릇도, 모두 내가 쥐여 준 것이다.", Style::Speech },
		{ kHero, "버려진 아이. 부모의 흔적조차 없던 아이. 처음부터…… 누구의 아이도 아니었다.", Style::Thought },
		{ "「   」", "그런데 너는 잿빛을 삼켰다. 판 밖으로 걸어 나온 말은, 치워야지.", Style::Speech },
		{ kHero, "판은 여기서 끝이다. 빛도 어둠도, 이제 서로를 미워할지 말지는 스스로 정할 것이다.", Style::Speech },
		{ nullptr, "두 손이 판 위로 내려왔다.", Style::Speech },
	};

	const Line kGodReturn[] =
	{
		{ "「   」", "다시 판 위에 섰구나. 말은 몇 번이고 다시 놓을 수 있지.", Style::Speech },
		{ kHero, "몇 번이든.", Style::Speech },
	};

	// It breaks as he did once: its own words lost to the same noise.
	const Line kGodFall[] =
	{
		{ nullptr, "빈 자리에 금이 갔다.", Style::Speech },
		{ "「   」", "말이…… 판을……", Style::Speech },
		{ "「   」", "[이 놀이는] {끝나}[지 않는다…… 판은 또 펼쳐진다……]", Style::Broken },
		{ nullptr, "구멍이 닫혔다. 하늘에는 아무것도 남지 않았다. 빈 자리조차.", Style::Speech },
		{ kHero, "몸 안의 무언가가 조용해졌다. 이제 빛과 어둠이 서로를 미워한다면, 그건 그들의 선택이다.", Style::Thought },
	};

	// ---- The endings ----

	const Line kEndingSavior[] =
	{
		{ nullptr, "최초의 잿빛의 존재가 쓰러지고, 세상에서 잿빛이 걷혔다.", Style::Speech },
		{ nullptr, "빛의 나라와 어둠의 나라는 경계에서 온 자를 구원자라 불렀다.", Style::Speech },
		{ nullptr, "그리고 그 해가 저물기 전에, 두 나라는 다시 전쟁을 시작했다.", Style::Speech },
		{ nullptr, "누군가 바라던 대로, 놀이는 다시 이어졌다.", Style::Speech },
	};

	const Line kEndingAsh[] =
	{
		{ nullptr, "그날, 경계에서 태어난 또 한 사람이 빛과 어둠을 버렸다.", Style::Speech },
		{ nullptr, "두 잿빛은 세상으로 번져 나갔다. 빛도 어둠도, 서로를 미워할 이유와 함께 사라져 갔다.", Style::Speech },
		{ nullptr, "평화는 오지 않았다. 남은 것은 고요뿐이었다.", Style::Speech },
		{ nullptr, "놀이판의 두 축이 지워지고, 놀이는 끝났다. 아무도 남지 않은 채로.", Style::Speech },
	};

	const Line kEndingScheme[] =
	{
		{ nullptr, "잿빛을 거절한 채, 진실만을 품고 걸음을 돌렸다.", Style::Speech },
		{ nullptr, "전쟁은 다시 시작되었다. 판을 벗어난 말이 무엇을 아는지, 판의 주인은 알고 있었다.", Style::Speech },
		{ nullptr, "어느 날부터 빛의 병사와 어둠의 병사가 같은 사람을 쫓기 시작했다. 아무도 이유를 묻지 않았다.", Style::Speech },
		{ nullptr, "판을 벗어난 말은 치워졌고, 놀이는 계속되었다.", Style::Speech },
	};

	const Line kEndingTrue[] =
	{
		{ nullptr, "신의 영역이 무너지고, 놀이판의 주인은 사라졌다.", Style::Speech },
		{ nullptr, "빛의 나라와 어둠의 나라는 처음으로, 이유 없이 상대를 죽이고 싶은 마음을 잃었다.", Style::Speech },
		{ nullptr, "남은 미움은 그들이 쌓아 온 것뿐이었다. 그것을 내려놓기까지는 오랜 세월이 걸릴 것이다.", Style::Speech },
		{ nullptr, "경계에서 태어난 자들은 더 이상 숨지 않았다. 빛과 어둠과 잿빛을 함께 지닌 한 사람이, 그 길 앞에 서 있었기에.", Style::Speech },
	};

	// No pronoun for him here, as in every ending: he is whoever was named.
	const Line kEpilogueTrue[] =
	{
		{ nullptr, "루키우스는 처음으로 경계로 향하는 길에 군대가 아닌 사절을 보냈다. 그 사절은 아무것도 되풀이하지 않았다.", Style::Speech },
		{ nullptr, "녹티스는 오랫동안 장부를 덮어 두었다. 다시 펼쳤을 때, 거기에는 빚을 갚아야 할 이름들이 적혀 있었다.", Style::Speech },
		{ nullptr, "피데스와 레퀴엠의 두 사람은 전선에서 다시 마주쳤다. 둘 다 칼을 뽑지 않았고, 그 오래된 기도를 더는 외우지 않았다.", Style::Speech },
		{ nullptr, "리미나 숲 가장자리, 이름 없는 무덤들 곁에 작은 집 한 채가 다시 섰다.", Style::Speech },
		{ nullptr, "그 집 앞에서, 숲에 버려졌던 아이는 처음으로 아무것도 거두지 않는 하루를 보냈다.", Style::Speech },
	};

	// In the order of Dialogue::EndingId.
	const Dialogue::Ending kEndings[Dialogue::EndingCount] =
	{
		{ Dialogue::EndingSavior, "구원자 엔딩", -1 },
		{ Dialogue::EndingAsh, "잿빛 엔딩", -1 },
		{ Dialogue::EndingScheme, "신의 계략 엔딩", -1 },
		{ Dialogue::EndingTrue, "진엔딩", Dialogue::EpilogueTrue },
	};

	// In the order of Dialogue::Id.
	const Script kScripts[Dialogue::Count] =
	{
		{ kPriestLight, CountOf(kPriestLight), kPriestLightChoices, CountOf(kPriestLightChoices) },
		{ kPriestOnGodLight, CountOf(kPriestOnGodLight), nullptr, 0 },
		{ kPriestErrorLight, CountOf(kPriestErrorLight), nullptr, 0 },
		{ kPriestDark, CountOf(kPriestDark), kPriestDarkChoices, CountOf(kPriestDarkChoices) },
		{ kPriestOnGodDark, CountOf(kPriestOnGodDark), nullptr, 0 },
		{ kPriestErrorDark, CountOf(kPriestErrorDark), nullptr, 0 },

		{ kRumorLucius, CountOf(kRumorLucius), nullptr, 0 },
		{ kRecognizedLucius, CountOf(kRecognizedLucius), nullptr, 0 },
		{ kRumorNoctis, CountOf(kRumorNoctis), nullptr, 0 },
		{ kRecognizedNoctis, CountOf(kRecognizedNoctis), nullptr, 0 },

		{ kCitizenLumen, CountOf(kCitizenLumen), nullptr, 0 },
		{ kCitizenTenebrae, CountOf(kCitizenTenebrae), nullptr, 0 },

		{ kLiminaElder, CountOf(kLiminaElder), nullptr, 0 },
		{ kLiminaChild, CountOf(kLiminaChild), nullptr, 0 },

		{ kPrologueOpening, CountOf(kPrologueOpening), nullptr, 0 },
		{ kPrologueRaid, CountOf(kPrologueRaid), nullptr, 0 },
		{ kPrologueAftermath, CountOf(kPrologueAftermath), nullptr, 0 },

		{ kKinisFirst, CountOf(kKinisFirst), nullptr, 0 },

		{ kKingLucius, CountOf(kKingLucius), nullptr, 0 },
		{ kKingLuciusAsk, CountOf(kKingLuciusAsk), kKingLuciusAskChoices, CountOf(kKingLuciusAskChoices) },
		{ kKingLuciusError, CountOf(kKingLuciusError), nullptr, 0 },
		{ kKingNoctis, CountOf(kKingNoctis), nullptr, 0 },
		{ kKingNoctisAsk, CountOf(kKingNoctisAsk), kKingNoctisAskChoices, CountOf(kKingNoctisAskChoices) },
		{ kKingNoctisError, CountOf(kKingNoctisError), nullptr, 0 },

		{ kRescueLight, CountOf(kRescueLight), nullptr, 0 },
		{ kRescueDark, CountOf(kRescueDark), nullptr, 0 },
		{ kSurvivorLight, CountOf(kSurvivorLight), nullptr, 0 },
		{ kSurvivorDark, CountOf(kSurvivorDark), nullptr, 0 },

		{ kEavesdropChosen, CountOf(kEavesdropChosen), nullptr, 0 },
		{ kEavesdropClouded, CountOf(kEavesdropClouded), nullptr, 0 },
		{ kEavesdropUnaware, CountOf(kEavesdropUnaware), nullptr, 0 },
		{ kChosenLater, CountOf(kChosenLater), nullptr, 0 },

		{ kRenewedWar, CountOf(kRenewedWar), nullptr, 0 },

		{ kKinisOffer, CountOf(kKinisOffer), kKinisOfferChoices, CountOf(kKinisOfferChoices) },
		{ kKinisAccept, CountOf(kKinisAccept), nullptr, 0 },
		{ kKinisRefuse, CountOf(kKinisRefuse), nullptr, 0 },
		{ kKinisSilent, CountOf(kKinisSilent), nullptr, 0 },
		{ kKinisLastBroken, CountOf(kKinisLastBroken), nullptr, 0 },
		{ kKinisLastClear, CountOf(kKinisLastClear), kKinisLastClearChoices, CountOf(kKinisLastClearChoices) },

		{ kGodMeeting, CountOf(kGodMeeting), nullptr, 0 },
		{ kGodReturn, CountOf(kGodReturn), nullptr, 0 },
		{ kGodFall, CountOf(kGodFall), nullptr, 0 },

		{ kEndingSavior, CountOf(kEndingSavior), nullptr, 0 },
		{ kEndingAsh, CountOf(kEndingAsh), nullptr, 0 },
		{ kEndingScheme, CountOf(kEndingScheme), nullptr, 0 },
		{ kEndingTrue, CountOf(kEndingTrue), nullptr, 0 },
		{ kEpilogueTrue, CountOf(kEpilogueTrue), nullptr, 0 },
	};
}

const Dialogue::Script& Dialogue::Get(int id)
{
	return kScripts[id];
}

int Dialogue::Resolve(int id, const Progress& progress)
{
	bool escaped = progress.Has(Story::EscapedGodInfluence);
	switch (id)
	{
	case PriestOnGodLight:
	{
		return escaped ? PriestErrorLight : PriestOnGodLight;
	}
	case PriestOnGodDark:
	{
		return escaped ? PriestErrorDark : PriestOnGodDark;
	}
	case RumorLucius:
	{
		return progress.recognizedByLucius ? RecognizedLucius : RumorLucius;
	}
	case RumorNoctis:
	{
		return progress.recognizedByNoctis ? RecognizedNoctis : RumorNoctis;
	}
	// Only once he has heard of the collusion is there anything to ask.
	case KingLucius:
	{
		bool ask = progress.Has(Story::KnowsCollusion) && !progress.Has(Story::AskedLucius);
		return ask ? KingLuciusAsk : KingLucius;
	}
	case KingNoctis:
	{
		bool ask = progress.Has(Story::KnowsCollusion) && !progress.Has(Story::AskedNoctis);
		return ask ? KingNoctisAsk : KingNoctis;
	}
	default:
	{
		return id;
	}
	}
}

void Dialogue::Finish(int id, Progress& progress)
{
	switch (id)
	{
	case KinisFirst:
	{
		progress.Set(Story::KnowsCollusion);
		break;
	}
	// One king failing to hear is strange; both is no accident.
	case KingLuciusError:
	case KingNoctisError:
	{
		progress.Set(Story::AskedKingAboutCollusion);
		if (progress.Has(Story::AskedLucius) && progress.Has(Story::AskedNoctis))
		{
			progress.Set(Story::SawKingAnomaly);
		}
		break;
	}
	case RescueLight:
	{
		progress.Set(Story::RescuedLight);
		break;
	}
	case RescueDark:
	{
		progress.Set(Story::RescuedDark);
		break;
	}
	case EavesdropChosen:
	case ChosenLater:
	{
		progress.Set(Story::KnowsOneGod);
		progress.Set(Story::TitleChosenByGod);
		progress.Set(Story::EscapedGodInfluence);
		break;
	}
	case EavesdropClouded:
	case EavesdropUnaware:
	{
		progress.Set(Story::KnowsOneGod);
		break;
	}
	case RenewedWar:
	{
		progress.Set(Story::SawRenewedWar);
		break;
	}
	case KinisLastBroken:
	case KinisLastClear:
	{
		progress.Set(Story::KnowsWorldTruth);
		break;
	}
	case GodMeeting:
	{
		progress.Set(Story::MetTheGod);
		break;
	}
	default:
	{
		break;
	}
	}
}

const Dialogue::Ending& Dialogue::GetEnding(int ending)
{
	return kEndings[ending];
}
