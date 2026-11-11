// mcpplibs.mwrap.boost.math — the official examples (libs/math/example/*.cpp, part 4 of 4),
// ported mechanically: each program runs as a case and its output must equal
// what the same code printed when built from upstream/boost headers alone. The
// headers of the macros the examples use (BOOST_MATH_DECLARE_*, BOOST_MATH_ASSERT,
// BOOST_FLOAT64_C, BOOST_SERIALIZATION_NVP) are #included before the imports.
// Outputs with random seeds, timings, the float layout or the C++/C library's
// text are not compared (random_search, inspect_fp, naive_monte_carlo,
// reverse_mode_linear_regression, differential_evolution; root_finding_example:
// std::cbrt's last digit; autodiff_reverse_black_scholes: last digits of libm results;
// nonfinite_num_facet_trap: the iostream_category message).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <boost/math/tools/config.hpp>
#include <boost/assert.hpp>
#include <boost/math/tools/assert.hpp>
#include <boost/math/cstdfloat/cstdfloat_types.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/distributions/fwd.hpp>
#include <boost/serialization/nvp.hpp>
#include <cassert>
#include <boost/config.hpp>
#include <cerrno>
#include <cmath>
import std;
import mcpplibs.mwrap.boost.math;
import mcpplibs.mwrap.boost.multiprecision;
import mcpplibs.mwrap.boost.lexical_cast;
import mcpplibs.mwrap.boost.array;
import mcpplibs.mwrap.boost.random;
import mcpplibs.mwrap.boost.range;
import mcpplibs.mwrap.boost.endian;
import mcpplibs.mwrap.boost.format;
import mcpplibs.mwrap.boost.serialization;
import mcpplibs.mwrap.boost.type_traits;

// Output of the ported examples is captured here; each case compares it with
// what the original program printed (built from upstream/boost alone).
inline std::ostringstream ex_out;
inline std::istringstream ex_in;
inline std::vector<std::string> ex_args;  // argv of the ported main
inline std::vector<char*> ex_argv_store;
inline char** ex_argv = nullptr;
inline int ex_argc = 0;
struct ex_argv_t {  // converts to char** and char const** (both main signatures)
    char** p;
    operator char**() const { return p; }
    operator const char**() const { return const_cast<const char**>(p); }
};
template <class... A>
int ex_printf(const char* fmt, A... a) {
    int n = std::snprintf(nullptr, 0, fmt, a...);
    std::string s(std::size_t(n) + 1, '\0');
    std::snprintf(s.data(), s.size(), fmt, a...);
    s.resize(std::size_t(n));
    ex_out << s;
    return n;
}
inline int ex_puts(const char* s) {
    ex_out << s << '\n';
    return 0;
}

// The examples write their SVG and data files to the current directory: the
// cases run in the temporary directory.
inline const bool ex_cwd_is_tmp = (std::filesystem::current_path(std::filesystem::temp_directory_path()), true);

// libs/math/example/arcsine_example.cpp
namespace ex_arcsine_example {
int example_main()
{
  ex_out << "Examples of Arcsine distribution." << std::endl;
  ex_out.precision(3);  // Avoid uninformative decimal digits.

  using boost::math::arcsine;

  arcsine as; // Construct a default `double` standard [0, 1] arcsine distribution.

//[arcsine_snip_2
  ex_out << pdf(as, 1. / 2) << std::endl; // 0.637
  // pdf has a minimum at x = 0.5
//]  [/arcsine_snip_2]

//[arcsine_snip_3
  ex_out << pdf(as, 1. / 4) << std::endl; // 0.735
//]  [/arcsine_snip_3]


//[arcsine_snip_4
  ex_out << cdf(as, 0.05) << std::endl; // 0.144
//] [/arcsine_snip_4]

//[arcsine_snip_5
  ex_out << 2 * cdf(as, 1 - 0.975) << std::endl; // 0.202
//] [/arcsine_snip_5]


//[arcsine_snip_6
  ex_out << 2 * cdf(complement(as, 0.975)) << std::endl; // 0.202
//] [/arcsine_snip_6]

//[arcsine_snip_7
  ex_out << quantile(as, 1 - 0.2 / 2) << std::endl; //  0.976

  ex_out << quantile(complement(as, 0.2 / 2)) << std::endl; // 0.976
//] [/arcsine_snip_7]

{
//[arcsine_snip_8
  using boost::math::arcsine_distribution;

  arcsine_distribution<> as(2, 5); // Constructs a double arcsine distribution.
  BOOST_MATH_ASSERT(as.x_min() == 2.);  // as.x_min() returns 2.
  BOOST_MATH_ASSERT(as.x_max() == 5.);   // as.x_max()  returns 5.
//] [/arcsine_snip_8]
}
    return 0;

    return 0;  // main's implicit return
} // int example_main()

/*
[arcsine_output

Example of Arcsine distribution
0.637
0.735
0.144
0.202
0.202
0.976
0.976

] [/arcsine_output]
*/

TEST_CASE("example: arcsine_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Examples of Arcsine distribution.
0.637
0.735
0.144
0.202
0.202
0.976
0.976
)ex"));
}
}  // namespace ex_arcsine_example

// libs/math/example/autodiff_mixed_partials.cpp
namespace ex_autodiff_mixed_partials {
using namespace boost::math::differentiation;

struct f {
  template <typename W, typename X, typename Y, typename Z>
  promote<W, X, Y, Z> operator()(W const& w, X const& x, Y const& y, Z const& z) const {
    return exp(w * sin(x * log(y) / z) + sqrt(w * z / (x * y))) + w * w / tan(z);
  }
};

// Derivatives calculated from symbolic differentiation by Mathematica for comparison. Script:
// mixed_partials.nb
static constexpr std::array<char const*, 240> answers{
    {"19878.40628980434922342465374997798674242532797789489",
     "20731.74838274939517275508122761443159515217855975002",
     "14667.60767623939014840117674691707821648144188283774",
     "1840.559936449813118734351750381849294157477519107602",
     "-9219.318005237072129605008516120710807803827373819700",
     "-7272.300634012811783845589472196110804386170683300081",
     "-2135.296370062283924160196772166043360841114107521292",
     "3095.081027251846799545897828297310835169325417217168",
     "4249.026762908615627428402369471953790564918480025345",
     "2063.989061062734416582172072883742097425754355167541",
     "-885.5284114876496084068555333811894392182458751895290",
     "-1962.133420441743158021558423645064067562765178375508",
     "-1846.899830787084518564013512948598850243350915531775",
     "-160.9590127603295755195950112199107484483554942817846",
     "1091.039412341633994110997652976585409621806446647794",
     "452.4395574345229946707651998323417632800605985181691",
     "666.4013922727704990031159406121675703174518834914461",
     "-415.6464114333629107803309520898363153301435468382605",
     "-625.1464179039986361267627631122900331946746137220517",
     "369.9491669772617110087494756677334192842413470837587",
     "-24330.89613849389343130420303653062335840497802221681",
     "-18810.41605175626752065686192937776868736029049989926",
     "-4890.406122702359099863022925593448420259414896197252",
     "8833.005054768976417065486877649473665597894570245307",
     "8484.350739681613747819854384228795938450532463850094",
     "3097.204151240398893507362023543393154680147349049848",
     "-3255.045136783440612110181337652522080890693968833148",
     "-4342.778553332193097878812792875447018366988006584840",
     "-2407.987237906523486012534085031032446996713414362131",
     "861.1173916470300084261504495377425043024739914571554",
     "2436.743725763308619092960749816106318692933687303014",
     "-19.24649610733827783846392798978023489104363382129689",
     "187.7855148870511714395275130898958731897480766620821",
     "-1259.466063335212195169531010871023748854744563232277",
     "-709.6860523972158261343923419671629587637051060458295",
     "1423.000558608604536932163648918899935569543711292466",
     "484.9208133389233959103861107714757012185008046446372",
     "763.9746885074453180462508029718247316712990115789154",
     "-327.4162918228055568224139277603073169658358026440432",
     "-1122.337707248494521123614369562896901904418640152220",
     "23973.06007192346989337502250398494874845408708506720",
     "8840.543151778796869949670401421984604862699128880003",
     "-9082.571033221549378277312292526023838132689941236879",
     "-12270.27378289258717737657881957466807305650429436397",
     "-4320.434071420599854743576892819691675331049612545664",
     "3281.351967707280898543984556670710235259118405463698",
     "5880.336263083418767219493592767818708317492833223933",
     "-1288.482785219706549809211085113790275109642879331959",
     "-803.9713537626580526627976840414468844364935388365037",
     "-2986.387245331698390346145949708414455858834967096376",
     "-586.7316859822658306283656047992829723003491823675739",
     "3929.073189280739356198769778905960586080418779863615",
     "1453.728280983826630077825553258703050898056317382483",
     "1037.878071685953829685046234106860743366780050925514",
     "-1482.745805277401336553926171580259185140208053329753",
     "-1877.134792933828810602377451370316364621357891989679",
     "-931.7138710369298207131581126980851620513905805624544",
     "254.6565590420322632851077818917210811815919344882311",
     "1391.248064745611663849820246430123214796614030838600",
     "-431.4820563154137955051720207563800896297257103310465",
     "16975.34005365179555009050533000516107937041784876054",
     "19662.60356303341709846238790020024593550984564081068",
     "15765.85130704020004301064240357947656083104783442825",
     "3972.155036195937013764185795634749937308876197976202",
     "-8681.748539789720512499473840242996096730194203989543",
     "-7703.183042460387656743498394861780784700076575106134",
     "-3049.708696569518774040135942468704911634779352213044",
     "2971.469685992270876159892302788930292108129670398058",
     "4370.196499857550025657084783894747734031876677385611",
     "2524.632473357435670756946837415389227139966527203701",
     "-656.6080000236679071742450437463693211275208125750923",
     "-2423.452917325258132591368397957959217829861665178601",
     "-2074.987664204263204162199830716851483704870169031179",
     "-381.2253794988132984501358802316138392247470857452486",
     "1219.507245791997351017860252538035146744682380716428",
     "805.3802239840836877339667281819652171888443003165988",
     "838.4004190058912380470543219448821914235443115661655",
     "-390.6125197108983831575656956558201636111305409512701",
     "-828.2085489298235758253219930356006757081473789845849",
     "293.8999854454994790079171865082094494146506490533363",
     "-22965.85985843951977785883587223006628792405076928067",
     "-20026.69101529929621743747554537576887048069629325374",
     "-7316.092745063355996548975300169565482331369744607021",
     "8632.466133972614659252310985982644793465043032940318",
     "8987.046882870452266200748127338744248816756004290490",
     "4199.925399536137541108783465785304128965582292174062",
     "-2958.429850896062893179851696175634522187021390095560",
     "-5665.563891218624062243686482808197054863235184904433",
     "-2945.404552250341615883104643651287431663294281737652",
     "555.6566272478262524735403145861484390537770707372992",
     "2936.796403550079139218970638242013974322758744804216",
     "651.5191650747110008135060635556227666232180743487328",
     "444.7629427486155148584918602702161457622049333694568",
     "-1390.989671799095801316658971275073184600067187023729",
     "-1142.861468946763860859271224968631944511098747155437",
     "1541.978723117340843491920690654997335632919116206279",
     "455.7146063293814470171599782651235242129856311098151",
     "998.7943503940357037260061331795191352937661538946216",
     "-204.8485581981121295383497187536442450324011940647949",
     "-1560.354115460478786113711476250386112014306509906244",
     "25278.29450605247223516529112562423587288781657290275",
     "11873.22337179046469888005044109378787446671408425048",
     "-8242.187303368878103323785658604027555126374435611949",
     "-15939.98056417465751946455567789306872745912255628512",
     "-5648.833539698031486810309720694416837861242341227280",
     "2751.513926122717118525029734574022921057261239749143",
     "7349.432002479077129245930487320138527887196396579062",
     "194.9972545980371127390142753318206783334452047502143",
     "-402.8156857682688265622049800462325595907987257153782",
     "-3518.871908683063371167722463713374376552181380727802",
     "-1494.304793474682619087166400375396721307777439607909",
     "4640.927509426080087451995953783429589632369803588940",
     "1585.757705203227141964561144798400703219894640413562",
     "1565.169992404407137888592924342582799362959736185298",
     "-1513.259809733540018859089666188672238777297615451800",
     "-2974.437872674680092826212901753475972242208819679978",
     "-1203.236292653823441598437153564865951527142648802876",
     "72.52425949879153384040698301599842998884036742649047",
     "1871.625274253419949517250818647194858608124560073483",
     "-2.489984337379681666361341362948045621969765070197429",
     "14462.74423518633102580192225823524237502860825596609",
     "18367.74740916432711689913219912502810575714860430297",
     "16565.76324499673961400925630526921000337443450249297",
     "6054.315252651102952034254100792777051580892954459740",
     "-8084.981271982030146065497115893934803061545998433631",
     "-7988.314359128201297240919364015959817416101519999194",
     "-3989.319346941492698525859335371231602272119870228687",
     "2616.721186534649016680934493970036169897788778926434",
     "4420.859270970486562095630193355634655337290952862363",
     "2973.033519764547909146474824627687039969488363657908",
     "-324.1453016982713707989332262410969595194473127209825",
     "-2843.242039958969221918101261762794653424879358390111",
     "-2281.461806143289517702658392470195144560150025832652",
     "-642.9353229582055924928927665183236308235598082837497",
     "1299.287274176955358490409470855361289523321919337117",
     "1238.597083372069762230817383681570828675426312803376",
     "1021.334042770848165110529668635291528449691525937968",
     "-329.0529345069271079573348500899329811170455711610811",
     "-1046.254301544052075124857362060924818517694048905299",
     "134.7343039554480655186788228552325941588620079791654",
     "-21431.41643507661192392650726158493697457993678274754",
     "-20856.88281479015784660571401663659059349708627445067",
     "-9829.261970591930907585958999196966814861251125275804",
     "7806.858647077811827981774785577363365546600234846335",
     "9319.700085649568180114405924685286453652118439999060",
     "5319.898768025758256383579171601100187435481641933401",
     "-2387.954826466841736373447020403170264502066930376059",
     "-6958.298525165359760665355886221309296550746152109847",
     "-3468.539106391972560670887295398968213297736424267559",
     "130.4167253342709401698825285623058661085645012029873",
     "3371.139930235175987370940343096776588915600470241960",
     "1569.232678004908105313880673484968847566948896728142",
     "750.0912101179065245750415609380442359608197763310413",
     "-1462.257209626597452197736652121394535208578921869658",
     "-1661.577809630240615684355192771059515041884351493459",
     "1509.628528603869133250456671040505284128185908768108",
     "383.8950902580816259502239917715884779698864996879279",
     "1248.051096343638013308778159911906703363730187986273",
     "17.18569564265260274901760034571610990094333217519021",
     "-2038.024598002604853054532645991188063394308018947374",
     "26118.98132017823514803387529120810044029492871875474",
     "14943.61943482227903328457116850255971625430735856355",
     "-6650.686262276131072415580833374348889422387492668440",
     "-19519.81529547404067945704333355155941895199228108631",
     "-6983.190236500848647457042860591724089812405118922223",
     "1899.297502873688983038424995203515277346497811783168",
     "8715.003652642963488202943622358986745434720576722170",
     "2368.150690681864301926962120618658083737878227231428",
     "136.8920793093482831910443246272238406481527839521448",
     "-3954.732706163417141961077488373290331419627965482785",
     "-2673.556440231186786375595871506657802723673830409989",
     "5078.483935249043594670125721926702845818403229980691",
     "1643.459143721204817182772630730123271413273760820347",
     "2182.216979506380293664703833586468523416961563720645",
     "-1345.838830963620501537777318021157952722412472356094",
     "-4309.285350629108413525304135326225818270616857298235",
     "-1488.050869922417817689426519211523527088509094291312",
     "-228.0584943070343720919835603886532454450555855354340",
     "2373.398940425709177876367020236623713151456855728138",
     "773.8481328103928058186643458500631723389600248582833",
     "12294.40387737855548614823173849184004455244840062464",
     "16977.34966571858301862913845572077593071467784570724",
     "17057.17475622503175013658695220988017704387344177727",
     "8121.189758511830935868344768490586007624092305459885",
     "-7458.443541406284389918808653948439156033975014107187",
     "-8134.131160882738058651976911725365291142418949378248",
     "-4912.881158613784419581465435995807691111897279859302",
     "2030.653136098933717888434825960516061206391833398177",
     "4407.490527709412730881592594976776779312299897714205",
     "3392.434568825892752350943548729559313328141534290860",
     "104.0372355841506198680609232049783930050635078746762",
     "-3180.817620484463214391157460812371170723810181051096",
     "-2460.523987075069437321629265332968914260047631079537",
     "-938.2209314069133432825590545267820890922150850657831",
     "1315.246905571876456706320919211807375254975062430487",
     "1735.862392405992188189147617586418269768276241147998",
     "1209.759657223166954850207025399731503326968841680649",
     "-227.3320054566642297128407910803774238020746116287390",
     "-1266.126209991929259396966729664100401813091860201682",
     "-123.0794572338149156803989321165094334755661021559442",
     "-19806.90794333834685506732819834090525250045748665845",
     "-21314.81635440575229337844631555492486744407550254908",
     "-12317.58384430130805020250005527399703840208659666608",
     "6349.418659888281474363154227419204673663621492760982",
     "9489.819687696527735093973063679592839666155440941289",
     "6409.538948456309994399374417972222747225748405617373",
     "-1550.281799013125267606263057621300789555474258987989",
     "-8109.711199785217512061886243157800006692908759687186",
     "-3957.840330296874877742767473517819198882831790006004",
     "-404.0796555836667858753163727999380679499192203780272",
     "3693.614351301181980145006883746936633676934626580499",
     "2716.146658322790064799415509615557123789406209068981",
     "1094.591086641398900496318896947912437274250932576747",
     "-1456.269645549946420883827817869876763706452982413420",
     "-2244.380608735636962338392373719455877272151458411079",
     "1268.593891556261871090883000459505759446497182073132",
     "265.2206730327749346649809229271069944357537135668622",
     "1496.091578778639488439197917198148587432113387871024",
     "354.6137351047722781932932090799444060236757625488818",
     "-2508.477110048684129181005769771219369377836598443263",
     "26517.86140875157324686379805134248778305979287686214",
     "17922.98387741915144079932445041215068937644694653527",
     "-4328.259142127668040873054918170572859673703425721293",
     "-22704.70245940080949074466622805971940616027152354999",
     "-8268.613747173738971390434576274225941735552759965376",
     "740.4056074392611464740778308961471299437619012164253",
     "9848.900182836035080973766381422758538530595451048714",
     "5213.598341476210337710365441072904970861063876340963",
     "801.2462923723508233330997243930793458484750729415321",
     "-4241.870133920767845856621968904769727964770527614244",
     "-4092.241355868550570635569815488217469506874233892269",
     "5074.435909206083943809967780457349942315503368249477",
     "1607.765329254820915989772546102530187884674235100928",
     "2861.155651116567526208762405651011317435252198548496",
     "-918.9310546317296090214320737728927500362088478158839",
     "-5803.211323646092019259074499814222806376618363553826",
     "-1767.541897994477314401145980308432268207111761980100",
     "-663.0646207520075726320417301262932382663072876188661",
     "2837.903194613938414496183429129769829434890424213252",
     "1976.319600747797717779881875290418720908121189218755"}};

int example_main() {
  using float50 = boost::multiprecision::cpp_bin_float_50;
  constexpr std::size_t Nw = 3;  // Max order of derivative to calculate for w
  constexpr std::size_t Nx = 2;  // Max order of derivative to calculate for x
  constexpr std::size_t Ny = 4;  // Max order of derivative to calculate for y
  constexpr std::size_t Nz = 3;  // Max order of derivative to calculate for z
  auto const variables = make_ftuple<float50, Nw, Nx, Ny, Nz>(11, 12, 13, 14);
  auto const v = boost::mp11::tuple_apply(f{}, variables);
  std::size_t ia = 0;
  double max_relative_error = 0;
  for (std::size_t iw = 0; iw <= Nw; ++iw)
    for (std::size_t ix = 0; ix <= Nx; ++ix)
      for (std::size_t iy = 0; iy <= Ny; ++iy)
        for (std::size_t iz = 0; iz <= Nz; ++iz) {
          float50 const value = v.derivative(iw, ix, iy, iz);
          float50 const answer = boost::lexical_cast<float50>(answers[ia++]);
          double const error = static_cast<double>(fabs(value / answer - 1));
          max_relative_error = (std::max)(error, max_relative_error);
        }
  ex_out << "max_relative_error = " << std::setprecision(3) << max_relative_error << " out of " << ia
            << " calculated values." << std::endl;
  return 0;
    return 0;  // main's implicit return
}
/*
Output:
max_relative_error = 6.82e-13 out of 240 calculated values. (for double)
max_relative_error = 3.36e-47 out of 240 calculated values. (for cpp_bin_float_50)
**/

TEST_CASE("example: autodiff_mixed_partials") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(max_relative_error = 1.44e-46 out of 240 calculated values.
)ex"));
}
}  // namespace ex_autodiff_mixed_partials

// libs/math/example/barycentric_interpolation_example_2.cpp
namespace ex_barycentric_interpolation_example_2 {
int example_main()
{
    // The lithium potential is given in Kohn's paper, Table I.
    // (We could equally easily use an unordered_map, a list of tuples or pairs, or a 2-dimensional array).
    std::map<double, double> r;

    r[0.02] = 5.727;
    r[0.04] = 5.544;
    r[0.06] = 5.450;
    r[0.08] = 5.351;
    r[0.10] = 5.253;
    r[0.12] = 5.157;
    r[0.14] = 5.058;
    r[0.16] = 4.960;
    r[0.18] = 4.862;
    r[0.20] = 4.762;
    r[0.24] = 4.563;
    r[0.28] = 4.360;
    r[0.32] = 4.1584;
    r[0.36] = 3.9463;
    r[0.40] = 3.7360;
    r[0.44] = 3.5429;
    r[0.48] = 3.3797;
    r[0.52] = 3.2417;
    r[0.56] = 3.1209;
    r[0.60] = 3.0138;
    r[0.68] = 2.8342;
    r[0.76] = 2.6881;
    r[0.84] = 2.5662;
    r[0.92] = 2.4242;
    r[1.00] = 2.3766;
    r[1.08] = 2.3058;
    r[1.16] = 2.2458;
    r[1.24] = 2.2035;
    r[1.32] = 2.1661;
    r[1.40] = 2.1350;
    r[1.48] = 2.1090;
    r[1.64] = 2.0697;
    r[1.80] = 2.0466;
    r[1.96] = 2.0325;
    r[2.12] = 2.0288;
    r[2.28] = 2.0292;
    r[2.44] = 2.0228;
    r[2.60] = 2.0124;
    r[2.76] = 2.0065;
    r[2.92] = 2.0031;
    r[3.08] = 2.0015;
    r[3.24] = 2.0008;
    r[3.40] = 2.0004;
    r[3.56] = 2.0002;
    r[3.72] = 2.0001;

    // Let's discover the abscissa that will generate a potential of exactly 3.0,
    // start by creating 2 ranges for the x and y values:
    auto x_range = boost::adaptors::keys(r);
    auto y_range = boost::adaptors::values(r);
    boost::math::interpolators::barycentric_rational<double> b(x_range.begin(), x_range.end(), y_range.begin());
    //
    // We'll use a lambda expression to provide the functor to our root finder, since we want
    // the abscissa value that yields 3, not zero.  We pass the functor b by value to the
    // lambda expression since barycentric_rational is trivial to copy.
    // Here we're using simple bisection to find the root:
    std::uintmax_t iterations = (std::numeric_limits<std::uintmax_t>::max)();
    double abscissa_3 = boost::math::tools::bisect([=](double x) { return b(x) - 3; }, 0.44, 1.24, boost::math::tools::eps_tolerance<double>(), iterations).first;
    ex_out << "Abscissa value that yields a potential of 3 = " << abscissa_3 << std::endl;
    ex_out << "Root was found in " << iterations << " iterations." << std::endl;
    //
    // However, we have a more efficient root finding algorithm than simple bisection:
    iterations = (std::numeric_limits<std::uintmax_t>::max)();
    abscissa_3 = boost::math::tools::bracket_and_solve_root([=](double x) { return b(x) - 3; }, 0.6, 1.2, false, boost::math::tools::eps_tolerance<double>(), iterations).first;
    ex_out << "Abscissa value that yields a potential of 3 = " << abscissa_3 << std::endl;
    ex_out << "Root was found in " << iterations << " iterations." << std::endl;
    return 0;  // main's implicit return
}
//] [/barycentric_rational_example2]


//[barycentric_rational_example2_out
/*` Program output is:
[pre
Abscissa value that yields a potential of 3 = 0.604728
Root was found in 54 iterations.
Abscissa value that yields a potential of 3 = 0.604728
Root was found in 10 iterations.
]
*/
//]

TEST_CASE("example: barycentric_interpolation_example_2") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Abscissa value that yields a potential of 3 = 0.604728
Root was found in 54 iterations.
Abscissa value that yields a potential of 3 = 0.604728
Root was found in 10 iterations.
)ex"));
}
}  // namespace ex_barycentric_interpolation_example_2

// libs/math/example/bessel_zeros_example_1.cpp
namespace ex_bessel_zeros_example_1 {
#ifdef _MSC_VER
#  pragma warning (disable : 4512) // assignment operator could not be generated.
#  pragma warning (disable : 4996) // assignment operator could not be generated.
#endif


// Weisstein, Eric W. "Bessel Function Zeros." From MathWorld--A Wolfram Web Resource.
// http://mathworld.wolfram.com/BesselFunctionZeros.html
// Test values can be calculated using [@wolframalpha.com WolframAlpha]
// See also http://dlmf.nist.gov/10.21

//[bessel_zeros_example_1

/*`This example demonstrates calculating zeros of the Bessel and Neumann functions.
It also shows how Boost.Math and Boost.Multiprecision can be combined to provide
a many decimal digit precision. For 50 decimal digit precision we need to include
*/


/*`and a `typedef` for `float_type` may be convenient
(allowing a quick switch to re-compute at built-in `double` or other precision)
*/
  typedef boost::multiprecision::cpp_dec_float_50 float_type;

//`To use the functions for finding zeros of the functions we need


//`This file includes the forward declaration signatures for the zero-finding functions:

//  #include <boost/math/special_functions/math_fwd.hpp>

/*`but more details are in the full documentation, for example at
[@http://www.boost.org/doc/libs/1_53_0/libs/math/doc/sf_and_dist/html/math_toolkit/special/bessel/bessel_over.html Boost.Math Bessel functions].
*/

/*`This example shows obtaining both a single zero of the Bessel function,
and then placing multiple zeros into a container like `std::vector` by providing an iterator.
*/
//] [/bessel_zeros_example_1]

/*The signature of the single value function is:

  template <class T>
  inline typename detail::bessel_traits<T, T, policies::policy<> >::result_type
    cyl_bessel_j_zero(
           T v,      // Floating-point value for Jv.
           int m);   // start index.

The result type is controlled by the floating-point type of parameter `v`
(but subject to the usual __precision_policy and __promotion_policy).

The signature of multiple zeros function is:

  template <class T, class OutputIterator>
  inline OutputIterator cyl_bessel_j_zero(
                                T v,                      // Floating-point value for Jv.
                                int start_index,          // 1-based start index.
                                unsigned number_of_zeros, // How many zeros to generate
                                OutputIterator out_it);   // Destination for zeros.

There is also a version which allows control of the __policy_section for error handling and precision.

  template <class T, class OutputIterator, class Policy>
  inline OutputIterator cyl_bessel_j_zero(
                                T v,                      // Floating-point value for Jv.
                                int start_index,          // 1-based start index.
                                unsigned number_of_zeros, // How many zeros to generate
                                OutputIterator out_it,    // Destination for zeros.
                                const Policy& pol);       // Policy to use.
*/

int example_main()
{
  try
  {
//[bessel_zeros_example_2

/*`[tip It is always wise to place code using Boost.Math inside try'n'catch blocks;
this will ensure that helpful error messages are shown when exceptional conditions arise.]

First, evaluate a single Bessel zero.

The precision is controlled by the float-point type of template parameter `T` of `v`
so this example has `double` precision, at least 15 but up to 17 decimal digits (for the common 64-bit double).
*/
//    double root = boost::math::cyl_bessel_j_zero(0.0, 1);
//    // Displaying with default precision of 6 decimal digits:
//    ex_out << "boost::math::cyl_bessel_j_zero(0.0, 1) " << root << std::endl; // 2.40483
//    // And with all the guaranteed (15) digits:
//    ex_out.precision(std::numeric_limits<double>::digits10);
//    ex_out << "boost::math::cyl_bessel_j_zero(0.0, 1) " << root << std::endl; // 2.40482555769577
/*`But note that because the parameter `v` controls the precision of the result,
`v` [*must be a floating-point type].
So if you provide an integer type, say 0, rather than 0.0, then it will fail to compile thus:
``
    root = boost::math::cyl_bessel_j_zero(0, 1);
``
with this error message
``
  error C2338: Order must be a floating-point type.
``

Optionally, we can use a policy to ignore errors, C-style, returning some value,
perhaps infinity or NaN, or the best that can be done. (See __user_error_handling).

To create a (possibly unwise!) policy `ignore_all_policy` that ignores all errors:
*/

  typedef boost::math::policies::policy<
    boost::math::policies::domain_error<boost::math::policies::ignore_error>,
    boost::math::policies::overflow_error<boost::math::policies::ignore_error>,
    boost::math::policies::underflow_error<boost::math::policies::ignore_error>,
    boost::math::policies::denorm_error<boost::math::policies::ignore_error>,
    boost::math::policies::pole_error<boost::math::policies::ignore_error>,
    boost::math::policies::evaluation_error<boost::math::policies::ignore_error>
              > ignore_all_policy;
 //`Examples of use of this `ignore_all_policy` are

    double inf = std::numeric_limits<double>::infinity();
    double nan = std::numeric_limits<double>::quiet_NaN();

    double dodgy_root = boost::math::cyl_bessel_j_zero(-1.0, 1, ignore_all_policy());
    ex_out << "boost::math::cyl_bessel_j_zero(-1.0, 1) " << dodgy_root << std::endl; // 1.#QNAN
    double inf_root = boost::math::cyl_bessel_j_zero(inf, 1, ignore_all_policy());
    ex_out << "boost::math::cyl_bessel_j_zero(inf, 1) " << inf_root << std::endl; // 1.#QNAN
    double nan_root = boost::math::cyl_bessel_j_zero(nan, 1, ignore_all_policy());
    ex_out << "boost::math::cyl_bessel_j_zero(nan, 1) " << nan_root << std::endl; // 1.#QNAN

/*`Another version of `cyl_bessel_j_zero`  allows calculation of multiple zeros with one call,
placing the results in a container, often `std::vector`.
For example, generate and display the first five `double` roots of J[sub v] for integral order 2,
as column ['J[sub 2](x)] in table 1 of
[@ http://mathworld.wolfram.com/BesselFunctionZeros.html Wolfram Bessel Function Zeros].
*/
    unsigned int n_roots = 5U;
    std::vector<double> roots;
    boost::math::cyl_bessel_j_zero(2.0, 1, n_roots, std::back_inserter(roots));
    std::copy(roots.begin(),
              roots.end(),
              std::ostream_iterator<double>(ex_out, "\n"));

/*`Or we can use Boost.Multiprecision to generate 50 decimal digit roots of ['J[sub v]]
for non-integral order `v= 71/19 == 3.736842`, expressed as an exact-integer fraction
to generate the most accurate value possible for all floating-point types.

We set the precision of the output stream, and show trailing zeros to display a fixed 50 decimal digits.
*/
    ex_out.precision(std::numeric_limits<float_type>::digits10); // 50 decimal digits.
    ex_out << std::showpoint << std::endl; // Show trailing zeros.

    float_type x = float_type(71) / 19;
    float_type r = boost::math::cyl_bessel_j_zero(x, 1); // 1st root.
    ex_out << "x = " << x << ", r = " << r << std::endl;

    r = boost::math::cyl_bessel_j_zero(x, 20U); // 20th root.
    ex_out << "x = " << x << ", r = " << r << std::endl;

    std::vector<float_type> zeros;
    boost::math::cyl_bessel_j_zero(x, 1, 3, std::back_inserter(zeros));

    ex_out << "cyl_bessel_j_zeros" << std::endl;
    // Print the roots to the output stream.
    std::copy(zeros.begin(), zeros.end(),
              std::ostream_iterator<float_type>(ex_out, "\n"));
//] [/bessel_zeros_example_2]
  }
  catch (std::exception const& ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }

     return 0;  // main's implicit return
} // int example_main()

 /*

 Output:

   Description: Autorun "J:\Cpp\big_number\Debug\bessel_zeros_example_1.exe"
  boost::math::cyl_bessel_j_zero(-1.0, 1) 3.83171
  boost::math::cyl_bessel_j_zero(inf, 1) 1.#QNAN
  boost::math::cyl_bessel_j_zero(nan, 1) 1.#QNAN
  5.13562
  8.41724
  11.6198
  14.796
  17.9598
  
  x = 3.7368421052631578947368421052631578947368421052632, r = 7.2731751938316489503185694262290765588963196701623
  x = 3.7368421052631578947368421052631578947368421052632, r = 67.815145619696290925556791375555951165111460585458
  cyl_bessel_j_zeros
  7.2731751938316489503185694262290765588963196701623
  10.724858308883141732536172745851416647110749599085
  14.018504599452388106120459558042660282427471931581

*/

TEST_CASE("example: bessel_zeros_example_1") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(boost::math::cyl_bessel_j_zero(-1.0, 1) 3.83171
boost::math::cyl_bessel_j_zero(inf, 1) nan
boost::math::cyl_bessel_j_zero(nan, 1) nan
5.13562
8.41724
11.6198
14.796
17.9598

x = 3.7368421052631578947368421052631578947368421052632, r = 7.2731751938316489503185694262290765588963196701623
x = 3.7368421052631578947368421052631578947368421052632, r = 67.815145619696290925556791375555951165111460585458
cyl_bessel_j_zeros
7.2731751938316489503185694262290765588963196701623
10.724858308883141732536172745851416647110749599085
14.018504599452388106120459558042660282427471931581
)ex"));
}
}  // namespace ex_bessel_zeros_example_1

// libs/math/example/binomial_confidence_limits.cpp
namespace ex_binomial_confidence_limits {
#ifdef _MSC_VER
#  pragma warning(disable: 4512) // assignment operator could not be generated.
#  pragma warning(disable: 4510) // default constructor could not be generated.
#  pragma warning(disable: 4610) // can never be instantiated - user defined constructor required.
#endif

 using std::endl;
using std::fixed; using std::left; using std::right; using std::right; using std::setw;
using std::setprecision;


void confidence_limits_on_frequency(unsigned trials, unsigned successes)
{
   //
   // trials = Total number of trials.
   // successes = Total number of observed successes.
   //
   // Calculate confidence limits for an observed
   // frequency of occurrence that follows a binomial distribution.
   //
   //using namespace std; // Avoid
   // using namespace boost::math; // potential name ambiguity with std <random>
   using boost::math::binomial_distribution;

   // Print out general info:
   ex_out <<
      "___________________________________________\n"
      "2-Sided Confidence Limits For Success Ratio\n"
      "___________________________________________\n\n";
   ex_out << setprecision(7);
   ex_out << setw(40) << left << "Number of Observations" << "=  " << trials << "\n";
   ex_out << setw(40) << left << "Number of successes" << "=  " << successes << "\n";
   ex_out << setw(40) << left << "Sample frequency of occurrence" << "=  " << double(successes) / trials << "\n";
   //
   // Define a table of significance levels:
   //
   double alpha[] = { 0.5, 0.25, 0.1, 0.05, 0.01, 0.001, 0.0001, 0.00001 };
   //
   // Print table header:
   //
   ex_out << "\n\n"
           "_______________________________________________________________________\n"
           "Confidence        Lower CP       Upper CP       Lower JP       Upper JP\n"
           " Value (%)        Limit          Limit          Limit          Limit\n"
           "_______________________________________________________________________\n";
   //
   // Now print out the data for the table rows.
   //
   for(unsigned i = 0; i < sizeof(alpha)/sizeof(alpha[0]); ++i)
   {
      // Confidence value:
      ex_out << fixed << setprecision(3) << setw(10) << right << 100 * (1-alpha[i]);
      // Calculate Clopper Pearson bounds:
      double l = binomial_distribution<>::find_lower_bound_on_p(trials, successes, alpha[i]/2);
      double u = binomial_distribution<>::find_upper_bound_on_p(trials, successes, alpha[i]/2);
      // Print Clopper Pearson Limits:
      ex_out << fixed << setprecision(5) << setw(15) << right << l;
      ex_out << fixed << setprecision(5) << setw(15) << right << u;
      // Calculate Jeffreys Prior Bounds:
      l = binomial_distribution<>::find_lower_bound_on_p(trials, successes, alpha[i]/2, binomial_distribution<>::jeffreys_prior_interval);
      u = binomial_distribution<>::find_upper_bound_on_p(trials, successes, alpha[i]/2, binomial_distribution<>::jeffreys_prior_interval);
      // Print Jeffreys Prior Limits:
      ex_out << fixed << setprecision(5) << setw(15) << right << l;
      ex_out << fixed << setprecision(5) << setw(15) << right << u << std::endl;
   }
   ex_out << endl;
} // void confidence_limits_on_frequency()

int example_main()
{
   confidence_limits_on_frequency(20, 4);
   confidence_limits_on_frequency(200, 40);
   confidence_limits_on_frequency(2000, 400);

   return 0;
    return 0;  // main's implicit return
} // int example_main()

/*

------ Build started: Project: binomial_confidence_limits, Configuration: Debug Win32 ------
Compiling...
binomial_confidence_limits.cpp
Linking...
Autorun "i:\boost-06-05-03-1300\libs\math\test\Math_test\debug\binomial_confidence_limits.exe"
___________________________________________
2-Sided Confidence Limits For Success Ratio
___________________________________________

Number of Observations                  =  20
Number of successes                     =  4
Sample frequency of occurrence          =  0.2


_______________________________________________________________________
Confidence        Lower CP       Upper CP       Lower JP       Upper JP
 Value (%)        Limit          Limit          Limit          Limit
_______________________________________________________________________
    50.000        0.12840        0.29588        0.14974        0.26916
    75.000        0.09775        0.34633        0.11653        0.31861
    90.000        0.07135        0.40103        0.08734        0.37274
    95.000        0.05733        0.43661        0.07152        0.40823
    99.000        0.03576        0.50661        0.04655        0.47859
    99.900        0.01905        0.58632        0.02634        0.55960
    99.990        0.01042        0.64997        0.01530        0.62495
    99.999        0.00577        0.70216        0.00901        0.67897

___________________________________________
2-Sided Confidence Limits For Success Ratio
___________________________________________

Number of Observations                  =  200
Number of successes                     =  40
Sample frequency of occurrence          =  0.2000000


_______________________________________________________________________
Confidence        Lower CP       Upper CP       Lower JP       Upper JP
 Value (%)        Limit          Limit          Limit          Limit
_______________________________________________________________________
    50.000        0.17949        0.22259        0.18190        0.22001
    75.000        0.16701        0.23693        0.16934        0.23429
    90.000        0.15455        0.25225        0.15681        0.24956
    95.000        0.14689        0.26223        0.14910        0.25951
    99.000        0.13257        0.28218        0.13468        0.27940
    99.900        0.11703        0.30601        0.11902        0.30318
    99.990        0.10489        0.32652        0.10677        0.32366
    99.999        0.09492        0.34485        0.09670        0.34197

___________________________________________
2-Sided Confidence Limits For Success Ratio
___________________________________________

Number of Observations                  =  2000
Number of successes                     =  400
Sample frequency of occurrence          =  0.2000000


_______________________________________________________________________
Confidence        Lower CP       Upper CP       Lower JP       Upper JP
 Value (%)        Limit          Limit          Limit          Limit
_______________________________________________________________________
    50.000        0.19382        0.20638        0.19406        0.20613
    75.000        0.18965        0.21072        0.18990        0.21047
    90.000        0.18537        0.21528        0.18561        0.21503
    95.000        0.18267        0.21821        0.18291        0.21796
    99.000        0.17745        0.22400        0.17769        0.22374
    99.900        0.17150        0.23079        0.17173        0.23053
    99.990        0.16658        0.23657        0.16681        0.23631
    99.999        0.16233        0.24169        0.16256        0.24143

*/

TEST_CASE("example: binomial_confidence_limits") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(___________________________________________
2-Sided Confidence Limits For Success Ratio
___________________________________________

Number of Observations                  =  20
Number of successes                     =  4
Sample frequency of occurrence          =  0.2


_______________________________________________________________________
Confidence        Lower CP       Upper CP       Lower JP       Upper JP
 Value (%)        Limit          Limit          Limit          Limit
_______________________________________________________________________
    50.000        0.12840        0.29588        0.14974        0.26916
    75.000        0.09775        0.34633        0.11653        0.31861
    90.000        0.07135        0.40103        0.08734        0.37274
    95.000        0.05733        0.43661        0.07152        0.40823
    99.000        0.03576        0.50661        0.04655        0.47859
    99.900        0.01905        0.58632        0.02634        0.55960
    99.990        0.01042        0.64997        0.01530        0.62495
    99.999        0.00577        0.70216        0.00901        0.67897

___________________________________________
2-Sided Confidence Limits For Success Ratio
___________________________________________

Number of Observations                  =  200
Number of successes                     =  40
Sample frequency of occurrence          =  0.2000000


_______________________________________________________________________
Confidence        Lower CP       Upper CP       Lower JP       Upper JP
 Value (%)        Limit          Limit          Limit          Limit
_______________________________________________________________________
    50.000        0.17949        0.22259        0.18190        0.22001
    75.000        0.16701        0.23693        0.16934        0.23429
    90.000        0.15455        0.25225        0.15681        0.24956
    95.000        0.14689        0.26223        0.14910        0.25951
    99.000        0.13257        0.28218        0.13468        0.27940
    99.900        0.11703        0.30601        0.11902        0.30318
    99.990        0.10489        0.32652        0.10677        0.32366
    99.999        0.09492        0.34485        0.09670        0.34197

___________________________________________
2-Sided Confidence Limits For Success Ratio
___________________________________________

Number of Observations                  =  2000
Number of successes                     =  400
Sample frequency of occurrence          =  0.2000000


_______________________________________________________________________
Confidence        Lower CP       Upper CP       Lower JP       Upper JP
 Value (%)        Limit          Limit          Limit          Limit
_______________________________________________________________________
    50.000        0.19382        0.20638        0.19406        0.20613
    75.000        0.18965        0.21072        0.18990        0.21047
    90.000        0.18537        0.21528        0.18561        0.21503
    95.000        0.18267        0.21821        0.18291        0.21796
    99.000        0.17745        0.22400        0.17769        0.22374
    99.900        0.17150        0.23079        0.17173        0.23053
    99.990        0.16658        0.23657        0.16681        0.23631
    99.999        0.16233        0.24169        0.16256        0.24143

)ex"));
}
}  // namespace ex_binomial_confidence_limits

// libs/math/example/c_error_policy_example.cpp
namespace ex_c_error_policy_example {
using boost::math::tgamma;

using boost::math::policies::policy;
// Possible errors
using boost::math::policies::overflow_error;
using boost::math::policies::underflow_error;
using boost::math::policies::domain_error;
using boost::math::policies::pole_error;
using boost::math::policies::denorm_error;
using boost::math::policies::evaluation_error;

using boost::math::policies::errno_on_error;
using boost::math::policies::ignore_error;

//using namespace boost::math::policies;
//using namespace boost::math; // avoid potential ambiguity with std:: <random>

// Define a policy:
typedef policy<
      domain_error<errno_on_error>, // 'bad' arguments.
      pole_error<errno_on_error>, // argument is pole value.
      overflow_error<errno_on_error>, // argument value causes overflow.
      evaluation_error<errno_on_error>  // evaluation does not converge and may be inaccurate, or worse,
      // or there is no way  known (yet) to implement this evaluation,
      // for example, kurtosis of non-central beta distribution.
      > C_error_policy;

// std
   
   using std::endl;   

int example_main()
{
  // We can achieve this at the function call site
  // with the previously defined policy C_error_policy.
  double t = tgamma(4., C_error_policy());
  ex_out << "tgamma(4., C_error_policy() = " << t << endl; // 6

  // Alternatively we could use the function make_policy,
  // provided for convenience,
  // and define everything at the call site:
  t = tgamma(4., make_policy(
         domain_error<errno_on_error>(), 
         pole_error<errno_on_error>(),
         overflow_error<errno_on_error>(),
         evaluation_error<errno_on_error>() 
      ));
  ex_out << "tgamma(4., make_policy(...) = " << t << endl; // 6

  return 0;
    return 0;  // main's implicit return
} // int example_main()

/*

Output

  c_error_policy_example.cpp
  Generating code
  Finished generating code
  c_error_policy_example.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Release\c_error_policy_example.exe
  tgamma(4., C_error_policy() = 6
  tgamma(4., make_policy(...) = 6
  tgamma(4., C_error_policy() = 6
  tgamma(4., make_policy(...) = 6

*/

TEST_CASE("example: c_error_policy_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(tgamma(4., C_error_policy() = 6
tgamma(4., make_policy(...) = 6
)ex"));
}
}  // namespace ex_c_error_policy_example

// libs/math/example/chi_square_std_dev_test.cpp
namespace ex_chi_square_std_dev_test {
 using std::endl;
using std::left; using std::fixed; using std::right; using std::scientific;
using std::setw;
using std::setprecision;


int error_result = 0;

void confidence_limits_on_std_deviation(
        double Sd,    // Sample Standard Deviation
        unsigned N)   // Sample size
{
   // Calculate confidence intervals for the standard deviation.
   // For example if we set the confidence limit to
   // 0.95, we know that if we repeat the sampling
   // 100 times, then we expect that the true standard deviation
   // will be between out limits on 95 occasions.
   // Note: this is not the same as saying a 95%
   // confidence interval means that there is a 95%
   // probability that the interval contains the true standard deviation.
   // The interval computed from a given sample either
   // contains the true standard deviation or it does not.
   // See http://www.itl.nist.gov/div898/handbook/eda/section3/eda358.htm

   // using namespace boost::math; // potential name ambiguity with std <random>
   using boost::math::chi_squared;
   using boost::math::quantile;
   using boost::math::complement;

   // Print out general info:
   ex_out <<
      "________________________________________________\n"
      "2-Sided Confidence Limits For Standard Deviation\n"
      "________________________________________________\n\n";
   ex_out << setprecision(7);
   ex_out << setw(40) << left << "Number of Observations" << "=  " << N << "\n";
   ex_out << setw(40) << left << "Standard Deviation" << "=  " << Sd << "\n";
   //
   // Define a table of significance/risk levels:
   double alpha[] = { 0.5, 0.25, 0.1, 0.05, 0.01, 0.001, 0.0001, 0.00001 };
   //
   // Start by declaring the distribution we'll need:
   chi_squared dist(N - 1);
   //
   // Print table header:
   //
   ex_out << "\n\n"
           "_____________________________________________\n"
           "Confidence          Lower          Upper\n"
           " Value (%)          Limit          Limit\n"
           "_____________________________________________\n";
   //
   // Now print out the data for the table rows.
   for(unsigned i = 0; i < sizeof(alpha)/sizeof(alpha[0]); ++i)
   {
      // Confidence value:
      ex_out << fixed << setprecision(3) << setw(10) << right << 100 * (1-alpha[i]);
      // Calculate limits:
      double lower_limit = sqrt((N - 1) * Sd * Sd / quantile(complement(dist, alpha[i] / 2)));
      double upper_limit = sqrt((N - 1) * Sd * Sd / quantile(dist, alpha[i] / 2));
      // Print Limits:
      ex_out << fixed << setprecision(5) << setw(15) << right << lower_limit;
      ex_out << fixed << setprecision(5) << setw(15) << right << upper_limit << endl;
   }
   ex_out << endl;
} // void confidence_limits_on_std_deviation

void confidence_limits_on_std_deviation_alpha(
        double Sd,    // Sample Standard Deviation
        double alpha  // confidence
        )
{  // Calculate confidence intervals for the standard deviation.
   // for the alpha parameter, for a range number of observations,
   // from a mere 2 up to a million.
   // O. L. Davies, Statistical Methods in Research and Production, ISBN 0 05 002437 X,
   // 4.33 Page 68, Table H, pp 452 459.

   //   using namespace std;
   // using namespace boost::math;
   using boost::math::chi_squared;
   using boost::math::quantile;
   using boost::math::complement;

   // Define a table of numbers of observations:
   unsigned int obs[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 30, 40 , 50, 60, 100, 120, 1000, 10000, 50000, 100000, 1000000};

   ex_out <<   // Print out heading:
      "________________________________________________\n"
      "2-Sided Confidence Limits For Standard Deviation\n"
      "________________________________________________\n\n";
   ex_out << setprecision(7);
   ex_out << setw(40) << left << "Confidence level (two-sided) " << "=  " << alpha << "\n";
   ex_out << setw(40) << left << "Standard Deviation" << "=  " << Sd << "\n";

   ex_out << "\n\n"      // Print table header:
            "_____________________________________________\n"
           "Observations        Lower          Upper\n"
           "                    Limit          Limit\n"
           "_____________________________________________\n";
    for(unsigned i = 0; i < sizeof(obs)/sizeof(obs[0]); ++i)
   {
     unsigned int N = obs[i]; // Observations
     // Start by declaring the distribution with the appropriate :
     chi_squared dist(N - 1);

     // Now print out the data for the table row.
      ex_out << fixed << setprecision(3) << setw(10) << right << N;
      // Calculate limits: (alpha /2 because it is a two-sided (upper and lower limit) test.
      double lower_limit = sqrt((N - 1) * Sd * Sd / quantile(complement(dist, alpha / 2)));
      double upper_limit = sqrt((N - 1) * Sd * Sd / quantile(dist, alpha / 2));
      // Print Limits:
      ex_out << fixed << setprecision(4) << setw(15) << right << lower_limit;
      ex_out << fixed << setprecision(4) << setw(15) << right << upper_limit << endl;
   }
   ex_out << endl;
}// void confidence_limits_on_std_deviation_alpha

void chi_squared_test(
       double Sd,     // Sample std deviation
       double D,      // True std deviation
       unsigned N,    // Sample size
       double alpha)  // Significance level
{
   //
   // A Chi Squared test applied to a single set of data.
   // We are testing the null hypothesis that the true
   // standard deviation of the sample is D, and that any variation is down
   // to chance.  We can also test the alternative hypothesis
   // that any difference is not down to chance.
   // See http://www.itl.nist.gov/div898/handbook/eda/section3/eda358.htm
   //
   // using namespace boost::math;
   using boost::math::chi_squared;
   using boost::math::quantile;
   using boost::math::complement;
   using boost::math::cdf;

   // Print header:
   ex_out <<
      "______________________________________________\n"
      "Chi Squared test for sample standard deviation\n"
      "______________________________________________\n\n";
   ex_out << setprecision(5);
   ex_out << setw(55) << left << "Number of Observations" << "=  " << N << "\n";
   ex_out << setw(55) << left << "Sample Standard Deviation" << "=  " << Sd << "\n";
   ex_out << setw(55) << left << "Expected True Standard Deviation" << "=  " << D << "\n\n";
   //
   // Now we can calculate and output some stats:
   //
   // test-statistic:
   double t_stat = (N - 1) * (Sd / D) * (Sd / D);
   ex_out << setw(55) << left << "Test Statistic" << "=  " << t_stat << "\n";
   //
   // Finally define our distribution, and get the probability:
   //
   chi_squared dist(N - 1);
   double p = cdf(dist, t_stat);
   ex_out << setw(55) << left << "CDF of test statistic: " << "=  "
      << setprecision(3) << scientific << p << "\n";
   double ucv = quantile(complement(dist, alpha));
   double ucv2 = quantile(complement(dist, alpha / 2));
   double lcv = quantile(dist, alpha);
   double lcv2 = quantile(dist, alpha / 2);
   ex_out << setw(55) << left << "Upper Critical Value at alpha: " << "=  "
      << setprecision(3) << scientific << ucv << "\n";
   ex_out << setw(55) << left << "Upper Critical Value at alpha/2: " << "=  "
      << setprecision(3) << scientific << ucv2 << "\n";
   ex_out << setw(55) << left << "Lower Critical Value at alpha: " << "=  "
      << setprecision(3) << scientific << lcv << "\n";
   ex_out << setw(55) << left << "Lower Critical Value at alpha/2: " << "=  "
      << setprecision(3) << scientific << lcv2 << "\n\n";
   //
   // Finally print out results of alternative hypothesis:
   //
   ex_out << setw(55) << left <<
      "Results for Alternative Hypothesis and alpha" << "=  "
      << setprecision(4) << fixed << alpha << "\n\n";
   ex_out << "Alternative Hypothesis              Conclusion\n";
   ex_out << "Standard Deviation != " << setprecision(3) << fixed << D << "            ";
   if((ucv2 < t_stat) || (lcv2 > t_stat))
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << "Standard Deviation  < " << setprecision(3) << fixed << D << "            ";
   if(lcv > t_stat)
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << "Standard Deviation  > " << setprecision(3) << fixed << D << "            ";
   if(ucv < t_stat)
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << endl << endl;
} // void chi_squared_test

void chi_squared_sample_sized(
        double diff,      // difference from variance to detect
        double variance)  // true variance
{
   using namespace std;
   // using boost::math;
   using boost::math::chi_squared;
   using boost::math::quantile;
   using boost::math::complement;
   using boost::math::cdf;

   try
   {
   ex_out <<   // Print out general info:
     "_____________________________________________________________\n"
      "Estimated sample sizes required for various confidence levels\n"
      "_____________________________________________________________\n\n";
   ex_out << setprecision(5);
   ex_out << setw(40) << left << "True Variance" << "=  " << variance << "\n";
   ex_out << setw(40) << left << "Difference to detect" << "=  " << diff << "\n";
   //
   // Define a table of significance levels:
   //
   double alpha[] = { 0.5, 0.33333333333333333333333, 0.25, 0.1, 0.05, 0.01, 0.001, 0.0001, 0.00001 };
   //
   // Print table header:
   //
   ex_out << "\n\n"
           "_______________________________________________________________\n"
           "Confidence       Estimated          Estimated\n"
           " Value (%)      Sample Size        Sample Size\n"
           "                (lower one-         (upper one-\n"
           "                 sided test)        sided test)\n"
           "_______________________________________________________________\n";
   //
   // Now print out the data for the table rows.
   //
   for(unsigned i = 0; i < sizeof(alpha)/sizeof(alpha[0]); ++i)
   {
      // Confidence value:
      ex_out << fixed << setprecision(3) << setw(10) << right << 100 * (1-alpha[i]);
      // Calculate df for a lower single-sided test:
      double df = chi_squared::find_degrees_of_freedom(
         -diff, alpha[i], alpha[i], variance);
      // Convert to integral sample size (df is a floating point value in this implementation):
      double size = ceil(df) + 1;
      // Print size:
      ex_out << fixed << setprecision(0) << setw(16) << right << size;
      // Calculate df for an upper single-sided test:
      df = chi_squared::find_degrees_of_freedom(
         diff, alpha[i], alpha[i], variance);
      // Convert to integral sample size:
      size = ceil(df) + 1;
      // Print size:
      ex_out << fixed << setprecision(0) << setw(16) << right << size << endl;
   }
   ex_out << endl;
   }
  catch(const std::exception& e)
  { // Always useful to include try & catch blocks because default policies
    // are to throw exceptions on arguments that cause errors like underflow, overflow.
    // Lacking try & catch blocks, the program will abort without a message below,
    // which may give some helpful clues as to the cause of the exception.
    ex_out <<
      "\n""Message from thrown exception was:\n   " << e.what() << std::endl;
    ++error_result;
  }
} // chi_squared_sample_sized

int example_main()
{
   // Run tests for Gear data
   // see http://www.itl.nist.gov/div898/handbook/eda/section3/eda3581.htm
   // Tests measurements of gear diameter.
   //
   confidence_limits_on_std_deviation(0.6278908E-02, 100);
   chi_squared_test(0.6278908E-02, 0.1, 100, 0.05);
   chi_squared_sample_sized(0.1 - 0.6278908E-02, 0.1);
   //
   // Run tests for silicon wafer fabrication data.
   // see http://www.itl.nist.gov/div898/handbook/prc/section2/prc23.htm
   // A supplier of 100 ohm.cm silicon wafers claims that his fabrication
   // process can produce wafers with sufficient consistency so that the
   // standard deviation of resistivity for the lot does not exceed
   // 10 ohm.cm. A sample of N = 10 wafers taken from the lot has a
   // standard deviation of 13.97 ohm.cm
   //
   confidence_limits_on_std_deviation(13.97, 10);
   chi_squared_test(13.97, 10.0, 10, 0.05);
   chi_squared_sample_sized(13.97 * 13.97 - 100, 100);
   chi_squared_sample_sized(55, 100);
   chi_squared_sample_sized(1, 100);

   // List confidence interval multipliers for standard deviation
   // for a range of numbers of observations from 2 to a million,
   // and for a few alpha values, 0.1, 0.05, 0.01 for confidences 90, 95, 99 %
   confidence_limits_on_std_deviation_alpha(1., 0.1);
   confidence_limits_on_std_deviation_alpha(1., 0.05);
   confidence_limits_on_std_deviation_alpha(1., 0.01);

   return error_result;
    return 0;  // main's implicit return
}

/*

________________________________________________
2-Sided Confidence Limits For Standard Deviation
________________________________________________
Number of Observations                  =  100
Standard Deviation                      =  0.006278908
_____________________________________________
Confidence          Lower          Upper
 Value (%)          Limit          Limit
_____________________________________________
    50.000        0.00601        0.00662
    75.000        0.00582        0.00685
    90.000        0.00563        0.00712
    95.000        0.00551        0.00729
    99.000        0.00530        0.00766
    99.900        0.00507        0.00812
    99.990        0.00489        0.00855
    99.999        0.00474        0.00895
______________________________________________
Chi Squared test for sample standard deviation
______________________________________________
Number of Observations                                 =  100
Sample Standard Deviation                              =  0.00628
Expected True Standard Deviation                       =  0.10000
Test Statistic                                         =  0.39030
CDF of test statistic:                                 =  1.438e-099
Upper Critical Value at alpha:                         =  1.232e+002
Upper Critical Value at alpha/2:                       =  1.284e+002
Lower Critical Value at alpha:                         =  7.705e+001
Lower Critical Value at alpha/2:                       =  7.336e+001
Results for Alternative Hypothesis and alpha           =  0.0500
Alternative Hypothesis              Conclusion
Standard Deviation != 0.100            NOT REJECTED
Standard Deviation  < 0.100            NOT REJECTED
Standard Deviation  > 0.100            REJECTED
_____________________________________________________________
Estimated sample sizes required for various confidence levels
_____________________________________________________________
True Variance                           =  0.10000
Difference to detect                    =  0.09372
_______________________________________________________________
Confidence       Estimated          Estimated
 Value (%)      Sample Size        Sample Size
                (lower one-         (upper one-
                 sided test)        sided test)
_______________________________________________________________
    50.000               2               2
    66.667               2               5
    75.000               2              10
    90.000               4              32
    95.000               5              52
    99.000               8             102
    99.900              13             178
    99.990              18             257
    99.999              23             337
________________________________________________
2-Sided Confidence Limits For Standard Deviation
________________________________________________
Number of Observations                  =  10
Standard Deviation                      =  13.9700000
_____________________________________________
Confidence          Lower          Upper
 Value (%)          Limit          Limit
_____________________________________________
    50.000       12.41880       17.25579
    75.000       11.23084       19.74131
    90.000       10.18898       22.98341
    95.000        9.60906       25.50377
    99.000        8.62898       31.81825
    99.900        7.69466       42.51593
    99.990        7.04085       55.93352
    99.999        6.54517       73.00132
______________________________________________
Chi Squared test for sample standard deviation
______________________________________________
Number of Observations                                 =  10
Sample Standard Deviation                              =  13.97000
Expected True Standard Deviation                       =  10.00000
Test Statistic                                         =  17.56448
CDF of test statistic:                                 =  9.594e-001
Upper Critical Value at alpha:                         =  1.692e+001
Upper Critical Value at alpha/2:                       =  1.902e+001
Lower Critical Value at alpha:                         =  3.325e+000
Lower Critical Value at alpha/2:                       =  2.700e+000
Results for Alternative Hypothesis and alpha           =  0.0500
Alternative Hypothesis              Conclusion
Standard Deviation != 10.000            REJECTED
Standard Deviation  < 10.000            REJECTED
Standard Deviation  > 10.000            NOT REJECTED
_____________________________________________________________
Estimated sample sizes required for various confidence levels
_____________________________________________________________
True Variance                           =  100.00000
Difference to detect                    =  95.16090
_______________________________________________________________
Confidence       Estimated          Estimated
 Value (%)      Sample Size        Sample Size
                (lower one-         (upper one-
                 sided test)        sided test)
_______________________________________________________________
    50.000               2               2
    66.667               2               5
    75.000               2              10
    90.000               4              32
    95.000               5              51
    99.000               7              99
    99.900              11             174
    99.990              15             251
    99.999              20             330
_____________________________________________________________
Estimated sample sizes required for various confidence levels
_____________________________________________________________
True Variance                           =  100.00000
Difference to detect                    =  55.00000
_______________________________________________________________
Confidence       Estimated          Estimated
 Value (%)      Sample Size        Sample Size
                (lower one-         (upper one-
                 sided test)        sided test)
_______________________________________________________________
    50.000               2               2
    66.667               4              10
    75.000               8              21
    90.000              23              71
    95.000              36             115
    99.000              71             228
    99.900             123             401
    99.990             177             580
    99.999             232             762
_____________________________________________________________
Estimated sample sizes required for various confidence levels
_____________________________________________________________
True Variance                           =  100.00000
Difference to detect                    =  1.00000
_______________________________________________________________
Confidence       Estimated          Estimated
 Value (%)      Sample Size        Sample Size
                (lower one-         (upper one-
                 sided test)        sided test)
_______________________________________________________________
    50.000               2               2
    66.667           14696           14993
    75.000           36033           36761
    90.000          130079          132707
    95.000          214283          218612
    99.000          428628          437287
    99.900          756333          771612
    99.990         1095435         1117564
    99.999         1440608         1469711
________________________________________________
2-Sided Confidence Limits For Standard Deviation
________________________________________________
Confidence level (two-sided)            =  0.1000000
Standard Deviation                      =  1.0000000
_____________________________________________
Observations        Lower          Upper
                    Limit          Limit
_____________________________________________
         2         0.5102        15.9472
         3         0.5778         4.4154
         4         0.6196         2.9200
         5         0.6493         2.3724
         6         0.6720         2.0893
         7         0.6903         1.9154
         8         0.7054         1.7972
         9         0.7183         1.7110
        10         0.7293         1.6452
        15         0.7688         1.4597
        20         0.7939         1.3704
        30         0.8255         1.2797
        40         0.8454         1.2320
        50         0.8594         1.2017
        60         0.8701         1.1805
       100         0.8963         1.1336
       120         0.9045         1.1203
      1000         0.9646         1.0383
     10000         0.9885         1.0118
     50000         0.9948         1.0052
    100000         0.9963         1.0037
   1000000         0.9988         1.0012
________________________________________________
2-Sided Confidence Limits For Standard Deviation
________________________________________________
Confidence level (two-sided)            =  0.0500000
Standard Deviation                      =  1.0000000
_____________________________________________
Observations        Lower          Upper
                    Limit          Limit
_____________________________________________
         2         0.4461        31.9102
         3         0.5207         6.2847
         4         0.5665         3.7285
         5         0.5991         2.8736
         6         0.6242         2.4526
         7         0.6444         2.2021
         8         0.6612         2.0353
         9         0.6755         1.9158
        10         0.6878         1.8256
        15         0.7321         1.5771
        20         0.7605         1.4606
        30         0.7964         1.3443
        40         0.8192         1.2840
        50         0.8353         1.2461
        60         0.8476         1.2197
       100         0.8780         1.1617
       120         0.8875         1.1454
      1000         0.9580         1.0459
     10000         0.9863         1.0141
     50000         0.9938         1.0062
    100000         0.9956         1.0044
   1000000         0.9986         1.0014
________________________________________________
2-Sided Confidence Limits For Standard Deviation
________________________________________________
Confidence level (two-sided)            =  0.0100000
Standard Deviation                      =  1.0000000
_____________________________________________
Observations        Lower          Upper
                    Limit          Limit
_____________________________________________
         2         0.3562       159.5759
         3         0.4344        14.1244
         4         0.4834         6.4675
         5         0.5188         4.3960
         6         0.5464         3.4848
         7         0.5688         2.9798
         8         0.5875         2.6601
         9         0.6036         2.4394
        10         0.6177         2.2776
        15         0.6686         1.8536
        20         0.7018         1.6662
        30         0.7444         1.4867
        40         0.7718         1.3966
        50         0.7914         1.3410
        60         0.8065         1.3026
       100         0.8440         1.2200
       120         0.8558         1.1973
      1000         0.9453         1.0609
     10000         0.9821         1.0185
     50000         0.9919         1.0082
    100000         0.9943         1.0058
   1000000         0.9982         1.0018
*/

TEST_CASE("example: chi_square_std_dev_test") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(________________________________________________
2-Sided Confidence Limits For Standard Deviation
________________________________________________

Number of Observations                  =  100
Standard Deviation                      =  0.006278908


_____________________________________________
Confidence          Lower          Upper
 Value (%)          Limit          Limit
_____________________________________________
    50.000        0.00601        0.00662
    75.000        0.00582        0.00685
    90.000        0.00563        0.00712
    95.000        0.00551        0.00729
    99.000        0.00530        0.00766
    99.900        0.00507        0.00812
    99.990        0.00489        0.00855
    99.999        0.00474        0.00895

______________________________________________
Chi Squared test for sample standard deviation
______________________________________________

Number of Observations                                 =  100
Sample Standard Deviation                              =  0.00628
Expected True Standard Deviation                       =  0.10000

Test Statistic                                         =  0.39030
CDF of test statistic:                                 =  1.438e-99
Upper Critical Value at alpha:                         =  1.232e+02
Upper Critical Value at alpha/2:                       =  1.284e+02
Lower Critical Value at alpha:                         =  7.705e+01
Lower Critical Value at alpha/2:                       =  7.336e+01

Results for Alternative Hypothesis and alpha           =  0.0500

Alternative Hypothesis              Conclusion
Standard Deviation != 0.100            NOT REJECTED
Standard Deviation  < 0.100            NOT REJECTED
Standard Deviation  > 0.100            REJECTED


_____________________________________________________________
Estimated sample sizes required for various confidence levels
_____________________________________________________________

True Variance                           =  0.10000
Difference to detect                    =  0.09372


_______________________________________________________________
Confidence       Estimated          Estimated
 Value (%)      Sample Size        Sample Size
                (lower one-         (upper one-
                 sided test)        sided test)
_______________________________________________________________
    50.000               2               2
    66.667               2               5
    75.000               2              10
    90.000               4              32
    95.000               5              52
    99.000               8             102
    99.900              13             178
    99.990              18             257
    99.999              23             337

________________________________________________
2-Sided Confidence Limits For Standard Deviation
________________________________________________

Number of Observations                  =  10
Standard Deviation                      =  13.9700000


_____________________________________________
Confidence          Lower          Upper
 Value (%)          Limit          Limit
_____________________________________________
    50.000       12.41880       17.25579
    75.000       11.23084       19.74131
    90.000       10.18898       22.98341
    95.000        9.60906       25.50377
    99.000        8.62898       31.81825
    99.900        7.69466       42.51593
    99.990        7.04085       55.93352
    99.999        6.54517       73.00132

______________________________________________
Chi Squared test for sample standard deviation
______________________________________________

Number of Observations                                 =  10
Sample Standard Deviation                              =  13.97000
Expected True Standard Deviation                       =  10.00000

Test Statistic                                         =  17.56448
CDF of test statistic:                                 =  9.594e-01
Upper Critical Value at alpha:                         =  1.692e+01
Upper Critical Value at alpha/2:                       =  1.902e+01
Lower Critical Value at alpha:                         =  3.325e+00
Lower Critical Value at alpha/2:                       =  2.700e+00

Results for Alternative Hypothesis and alpha           =  0.0500

Alternative Hypothesis              Conclusion
Standard Deviation != 10.000            REJECTED
Standard Deviation  < 10.000            REJECTED
Standard Deviation  > 10.000            NOT REJECTED


_____________________________________________________________
Estimated sample sizes required for various confidence levels
_____________________________________________________________

True Variance                           =  100.00000
Difference to detect                    =  95.16090


_______________________________________________________________
Confidence       Estimated          Estimated
 Value (%)      Sample Size        Sample Size
                (lower one-         (upper one-
                 sided test)        sided test)
_______________________________________________________________
    50.000               2               2
    66.667               2               5
    75.000               2              10
    90.000               4              32
    95.000               5              51
    99.000               7              99
    99.900              11             174
    99.990              15             251
    99.999              20             330

_____________________________________________________________
Estimated sample sizes required for various confidence levels
_____________________________________________________________

True Variance                           =  100.00000
Difference to detect                    =  55.00000


_______________________________________________________________
Confidence       Estimated          Estimated
 Value (%)      Sample Size        Sample Size
                (lower one-         (upper one-
                 sided test)        sided test)
_______________________________________________________________
    50.000               2               2
    66.667               4              10
    75.000               8              21
    90.000              23              71
    95.000              36             115
    99.000              71             228
    99.900             123             401
    99.990             177             580
    99.999             232             762

_____________________________________________________________
Estimated sample sizes required for various confidence levels
_____________________________________________________________

True Variance                           =  100.00000
Difference to detect                    =  1.00000


_______________________________________________________________
Confidence       Estimated          Estimated
 Value (%)      Sample Size        Sample Size
                (lower one-         (upper one-
                 sided test)        sided test)
_______________________________________________________________
    50.000               2               2
    66.667           14696           14993
    75.000           36033           36761
    90.000          130079          132707
    95.000          214283          218612
    99.000          428628          437287
    99.900          756333          771612
    99.990         1095435         1117564
    99.999         1440608         1469711

________________________________________________
2-Sided Confidence Limits For Standard Deviation
________________________________________________

Confidence level (two-sided)            =  0.1000000
Standard Deviation                      =  1.0000000


_____________________________________________
Observations        Lower          Upper
                    Limit          Limit
_____________________________________________
         2         0.5102        15.9472
         3         0.5778         4.4154
         4         0.6196         2.9200
         5         0.6493         2.3724
         6         0.6720         2.0893
         7         0.6903         1.9154
         8         0.7054         1.7972
         9         0.7183         1.7110
        10         0.7293         1.6452
        15         0.7688         1.4597
        20         0.7939         1.3704
        30         0.8255         1.2797
        40         0.8454         1.2320
        50         0.8594         1.2017
        60         0.8701         1.1805
       100         0.8963         1.1336
       120         0.9045         1.1203
      1000         0.9646         1.0383
     10000         0.9885         1.0118
     50000         0.9948         1.0052
    100000         0.9963         1.0037
   1000000         0.9988         1.0012

________________________________________________
2-Sided Confidence Limits For Standard Deviation
________________________________________________

Confidence level (two-sided)            =  0.0500000
Standard Deviation                      =  1.0000000


_____________________________________________
Observations        Lower          Upper
                    Limit          Limit
_____________________________________________
         2         0.4461        31.9102
         3         0.5207         6.2847
         4         0.5665         3.7285
         5         0.5991         2.8736
         6         0.6242         2.4526
         7         0.6444         2.2021
         8         0.6612         2.0353
         9         0.6755         1.9158
        10         0.6878         1.8256
        15         0.7321         1.5771
        20         0.7605         1.4606
        30         0.7964         1.3443
        40         0.8192         1.2840
        50         0.8353         1.2461
        60         0.8476         1.2197
       100         0.8780         1.1617
       120         0.8875         1.1454
      1000         0.9580         1.0459
     10000         0.9863         1.0141
     50000         0.9938         1.0062
    100000         0.9956         1.0044
   1000000         0.9986         1.0014

________________________________________________
2-Sided Confidence Limits For Standard Deviation
________________________________________________

Confidence level (two-sided)            =  0.0100000
Standard Deviation                      =  1.0000000


_____________________________________________
Observations        Lower          Upper
                    Limit          Limit
_____________________________________________
         2         0.3562       159.5759
         3         0.4344        14.1244
         4         0.4834         6.4675
         5         0.5188         4.3960
         6         0.5464         3.4848
         7         0.5688         2.9798
         8         0.5875         2.6601
         9         0.6036         2.4394
        10         0.6177         2.2776
        15         0.6686         1.8536
        20         0.7018         1.6662
        30         0.7444         1.4867
        40         0.7718         1.3966
        50         0.7914         1.3410
        60         0.8065         1.3026
       100         0.8440         1.2200
       120         0.8558         1.1973
      1000         0.9453         1.0609
     10000         0.9821         1.0185
     50000         0.9919         1.0082
    100000         0.9943         1.0058
   1000000         0.9982         1.0018

)ex"));
}
}  // namespace ex_chi_square_std_dev_test

// libs/math/example/differential_evolution.cpp
namespace ex_differential_evolution {
using boost::math::optimization::differential_evolution_parameters;
using boost::math::optimization::differential_evolution;

double rosenbrock(std::vector<double> const & x) {
   double result = 0;
   for (size_t i = 0; i < x.size() - 1; ++i) {
       double tmp = x[i+1] - x[i]*x[i];
       result += 100*tmp*tmp + (1-x[i])*(1-x[i]); 
   }
   return result;
}

int example_main() {
    auto de_params = differential_evolution_parameters<std::vector<double>>();
    constexpr const size_t dimension = 10;
    // Search on [0, 2]^dimension:
    de_params.lower_bounds.resize(dimension, 0);
    de_params.upper_bounds.resize(dimension, 2);
    // This is a challenging function, increase the max generations 10x from default so we don't terminate prematurely:
    de_params.max_generations *= 10;
    std::random_device rd;
    std::mt19937_64 rng(rd());
    // The global minima is exactly zero-but some leeway is required:
    double value_to_reach = 1e-5;
    auto local_minima = differential_evolution(rosenbrock, de_params, rng, value_to_reach);
    ex_out << "Minima: {";
    for (auto l : local_minima) {
        ex_out << l << ", ";
    }
    ex_out << "}\n";
    ex_out << "Value of cost function at minima: " << rosenbrock(local_minima) << "\n";
    return 0;  // main's implicit return
}

TEST_CASE("example: differential_evolution") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Minima: {0.999937, 0.999959, 1.00002, 1.00003, 1.00009, 1.00008, 1.00003, 0.999985, 0.999871, 0.999723, }
Value of cost function at minima: 6.05794e-06
)ex"));
#endif
}
}  // namespace ex_differential_evolution

// libs/math/example/error_policy_example.cpp
namespace ex_error_policy_example {
using boost::math::students_t_distribution;  // Probability of students_t(df, t).
using boost::math::students_t;  // Probability of students_t(df, t) convenience typedef for double.

using boost::math::policies::policy;
using boost::math::policies::domain_error;
using boost::math::policies::ignore_error;

// std
   
   using std::endl;

   

// Define a (bad?) policy to ignore domain errors ('bad' arguments):
typedef policy<
      domain_error<ignore_error>
      > my_policy;

// Define my_students_t distribution with this different domain error policy:
typedef students_t_distribution<double, my_policy> my_students_t;

int example_main()
{  // Example of error handling of bad argument(s) to a distribution.
  ex_out << "Example error handling using Student's t function. " << endl;

  double degrees_of_freedom = -1; double t = -1.; // Two 'bad' arguments!

  try
  {
    ex_out << "Probability of ignore_error Student's t is "
      << cdf(my_students_t(degrees_of_freedom), t) << endl;
    ex_out << "Probability of default error policy Student's t is " << endl;
    // By contrast the students_t distribution default domain error policy is to throw,
    ex_out << cdf(students_t(-1), -1) << endl;  // so this will throw.
/*`
    Message from thrown exception was:
   Error in function boost::math::students_t_distribution<double>::students_t_distribution:
   Degrees of freedom argument is -1, but must be > 0 !
*/

    // We could also define a 'custom' distribution
    // with an "ignore overflow error policy" in a single statement:
    using boost::math::policies::overflow_error;
    students_t_distribution<double, policy<overflow_error<ignore_error> > > students_t_no_throw(-1);

  }
  catch(const std::exception& e)
  {
    ex_out <<
      "\n""Message from thrown exception was:\n   " << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
} // int example_main()

/*

Output:

   error_policy_example.cpp
  Generating code
  Finished generating code
  error_policy_example.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Release\error_policy_example.exe
  Example error handling using Student's t function. 
  Probability of ignore_error Student's t is 1.#QNAN
  Probability of default error policy Student's t is 
  
  Message from thrown exception was:
     Error in function boost::math::students_t_distribution<double>::students_t_distribution: Degrees of freedom argument is -1, but must be > 0 !

*/

TEST_CASE("example: error_policy_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Example error handling using Student's t function. 
Probability of ignore_error Student's t is nan
Probability of default error policy Student's t is 

Message from thrown exception was:
   Error in function boost::math::students_t_distribution<double>::students_t_distribution: Degrees of freedom argument is -1, but must be > 0 !
)ex"));
}
}  // namespace ex_error_policy_example

// libs/math/example/fft_sines_table.cpp
namespace ex_fft_sines_table {
#ifdef _MSC_VER
#  pragma warning (disable : 4996)  // -D_SCL_SECURE_NO_WARNINGS.
#endif

//[fft_sines_table_example_1

/*`[h5 Using Boost.Multiprecision to generate a high-precision array of sine coefficients for use with FFT.]

The Boost.Multiprecision library can be used for computations requiring precision
exceeding that of standard built-in types such as `float`, `double`
and `long double`. For extended-precision calculations, Boost.Multiprecision
supplies a template data type called `cpp_bin_float`. The number of decimal
digits of precision is fixed at compile-time via a template parameter.

One often needs to compute tables of numbers in mathematical software.
To avoid the
[@https://en.wikipedia.org/wiki/Rounding#Table-maker's_dilemma Table-maker's dilemma]
it is necessary to use a higher precision type to compute the table values so that they have
the nearest representable bit-pattern for the type, say `double`, of the table value.

This example is a program `fft_since_table.cpp` that writes a header file `sines.hpp`
containing an array of sine coefficients for use with a Fast Fourier Transform (FFT),
that can be included by the FFT program.

To use Boost.Multiprecision's high-precision floating-point types and constants, we need some includes:
*/
// using boost::math::constants::pi;

// using boost::multiprecision::cpp_bin_float and
// using boost::multiprecision::cpp_bin_float_50;
// using boost::multiprecision::cpp_bin_float_quad;



/*`First, this example defines a prolog text string which is a C++ comment with the program licence, copyright etc.
(You would of course, tailor this to your needs, including *your* copyright claim).
This will appear at the top of the written header file `sines.hpp`.
*/

//] [fft_sines_table_example_1]

static const char* prolog =
{
  "// Use, modification and distribution are subject to the\n"
  "// Boost Software License, Version 1.0.\n"
  "// (See accompanying file LICENSE_1_0.txt\n"
  "// or copy at ""http://www.boost.org/LICENSE_1_0.txt)\n\n"

  "// Copyright A N Other, 2019.\n\n"
};

//[fft_sines_table_example_2

using boost::multiprecision::cpp_bin_float_50;
using boost::math::constants::pi;

//] [fft_sines_table_example_2]

// VS 2010 (wrongly) requires these at file scope, not local scope in `main`.
// This program also requires `-std=c++11` option to compile using Clang and GCC.

int example_main()
{
//[fft_sines_table_example_3
/*`A fast Fourier transform (FFT), for example, may use a table of the values of
sin(([pi]/2[super n]) in its implementation details. In order to maximize the precision in
the FFT implementation, the precision of the tabulated trigonometric values
should exceed that of the built-in floating-point type used in the FFT.

The sample below computes a table of the values of sin([pi]/2[super n])
in the range 1  <= n <= 31.

This program makes use of, among other program elements, the data type
`boost::multiprecision::cpp_bin_float_50`
for a precision of 50 decimal digits from Boost.Multiprecision,
the value of constant [pi] retrieved from Boost.Math,
guaranteed to be initialized with the very last bit of precision for the type,
here `cpp_bin_float_50`,
and a C++11 lambda function combined with `std::for_each()`.
*/

/*`define the number of values (32) in the array of sines.
*/

  std::size_t size = 32U;
  //cpp_bin_float_50 p = pi<cpp_bin_float_50>();
  cpp_bin_float_50 p = boost::math::constants::pi<cpp_bin_float_50>();

  std::vector <cpp_bin_float_50> sin_values (size);
  unsigned n = 1U;
  // Generate the sine values.
  std::for_each
  (
    sin_values.begin (),
    sin_values.end (),
    [&n](cpp_bin_float_50& y)
    {
      y = sin( pi<cpp_bin_float_50>() / pow(cpp_bin_float_50 (2), n));
      ++n;
    }
  );

/*`Define the floating-point type for the generated file, either built-in
`double, `float, or `long double`, or a user defined type like `cpp_bin_float_50`.
*/

std::string fp_type = "double";

ex_out << "Generating an `std::array` or `std::array` for floating-point type: "
  << fp_type << ". " << std::endl;

/*`By default, output would only show the standard 6 decimal digits,
so set precision to show enough significant digits for the chosen floating-point type.
For `cpp_bin_float_50` is 50. (50 decimal digits should be ample for most applications).

*/
  std::streamsize precision = std::numeric_limits<cpp_bin_float_50>::digits10;

  ex_out << "Sines table precision is " << precision << " decimal digits. " << std::endl;

/*`Of course, one could also choose a lower precision for the table values, for example,

`std::streamsize precision = std::numeric_limits<cpp_bin_float_quad>::max_digits10;`

128-bit 'quad' precision of 36 decimal digits would be sufficient
for the most precise current `long double` implementations using 128-bit.
In general, it should be a couple of decimal digits more (guard digits) than
`std::numeric_limits<RealType>::max_digits10` for the target system floating-point type.
(If the implementation does not provide `max_digits10`, the the Kahan formula
`std::numeric_limits<RealType>::digits * 3010/10000 + 2` can be used instead).

The compiler will read these values as decimal digits strings and
use the nearest representation for the floating-point type.

Now output all the sine table, to a file of your chosen name.
*/
  const char sines_name[] = "sines.hpp";  // Assuming in same directory as .exe

  std::ofstream fout(sines_name, std::ios_base::out);  // Creates if no file exists,
  // & uses default overwrite/ ios::replace.
  if (fout.is_open() == false)
  {  // failed to open OK!
    ex_out << "Open file " << sines_name << " failed!" << std::endl;
    return EXIT_FAILURE;
  }
  else
  { // Write prolog etc as a C++ comment.
    ex_out << "Open file " << sines_name << " for output OK." << std::endl;
    fout << prolog
    << "// Table of " << sin_values.size() << " values with "
      << precision << " decimal digits precision,\n"
      "// generated by program fft_sines_table.cpp.\n" << std::endl;

  fout << "#include <array> // std::array" << std::endl;

  // Write the table of sines as a C++ array.
    fout <<  "\nstatic const std::array<double, " << size << "> sines =\n"
    "{{\n"; // 2nd { needed for some old GCC compiler versions.
    fout.precision(precision);

    for (unsigned int i = 0U; ;)
    {
      fout << "  " << sin_values[i];
      if (i == sin_values.size()-1)
      { // next is last value.
        fout << "\n}};  // array sines\n"; // 2nd } needed for some old GCC compiler versions.
        break;
      }
      else
      {
        fout << ",\n";
        i++;
      }
    } // for

    fout.close();
    ex_out << "Closed file " << sines_name << " for output." << std::endl;
  }
//`The output file generated can be seen at [@../../example/sines.hpp]

//] [/fft_sines_table_example_3]

  return EXIT_SUCCESS;

    return 0;  // main's implicit return
} // int example_main()

/*
//[fft_sines_table_example_output

The printed table is:

  1
  0.70710678118654752440084436210484903928483593768847
  0.38268343236508977172845998403039886676134456248563
  0.19509032201612826784828486847702224092769161775195
  0.098017140329560601994195563888641845861136673167501
  0.049067674327418014254954976942682658314745363025753
  0.024541228522912288031734529459282925065466119239451
  0.012271538285719926079408261951003212140372319591769
  0.0061358846491544753596402345903725809170578863173913
  0.003067956762965976270145365490919842518944610213452
  0.0015339801862847656123036971502640790799548645752374
  0.00076699031874270452693856835794857664314091945206328
  0.00038349518757139558907246168118138126339502603496474
  0.00019174759731070330743990956198900093346887403385916
  9.5873799095977345870517210976476351187065612851145e-05
  4.7936899603066884549003990494658872746866687685767e-05
  2.3968449808418218729186577165021820094761474895673e-05
  1.1984224905069706421521561596988984804731977538387e-05
  5.9921124526424278428797118088908617299871778780951e-06
  2.9960562263346607504548128083570598118251878683408e-06
  1.4980281131690112288542788461553611206917585861527e-06
  7.4901405658471572113049856673065563715595930217207e-07
  3.7450702829238412390316917908463317739740476297248e-07
  1.8725351414619534486882457659356361712045272098287e-07
  9.3626757073098082799067286680885620193236507169473e-08
  4.681337853654909269511551813854009695950362701667e-08
  2.3406689268274552759505493419034844037886207223779e-08
  1.1703344634137277181246213503238103798093456639976e-08
  5.8516723170686386908097901008341396943900085051757e-09
  2.9258361585343193579282304690689559020175857150074e-09
  1.4629180792671596805295321618659637103742615227834e-09
*/

//]  [/fft_sines_table_example_output]

//[fft_sines_table_example_check

/*`
The output can be copied as text and readily integrated into a given source
code. Alternatively, the output can be written to a text or even be used
within a self-written automatic code generator as this example.

A computer algebra system can be used to verify the results obtained from
Boost.Math and Boost.Multiprecision. For example, the __Mathematica
computer algebra system can obtain a similar table with the command:

  Table[N[Sin[Pi / (2^n)], 50], {n, 1, 31, 1}]

The __WolframAlpha computational knowledge engine can also be used to generate
this table. The same command can be pasted into the compute box.

*/

//] [/fft_sines_table_example_check]

TEST_CASE("example: fft_sines_table") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Generating an `std::array` or `std::array` for floating-point type: double. 
Sines table precision is 50 decimal digits. 
Open file sines.hpp for output OK.
Closed file sines.hpp for output.
)ex"));
}
}  // namespace ex_fft_sines_table

// libs/math/example/find_scale_example.cpp
namespace ex_find_scale_example {
  using boost::math::normal; // typedef provides default type is double.
  using boost::math::find_scale;
  using boost::math::complement; // Needed if you want to use the complement version.
  using boost::math::policies::policy; // Needed to specify the error handling policy.

   using std::endl;
  using std::setw; using std::setprecision;
  using std::numeric_limits;
//] [/find_scale1]

int example_main()
{
  ex_out << "Example: Find scale (standard deviation)." << endl;
  try
  {
//[find_scale2
/*`
For this example, we will use the standard __normal_distrib,
with location (mean) zero and standard deviation (scale) unity.
Conveniently, this is also the default for this implementation's constructor.
*/
  normal N01;  // Default 'standard' normal distribution with zero mean
  double sd = 1.; // and standard deviation is 1.
/*`Suppose we want to find a different normal distribution with standard deviation
so that only fraction p (here 0.001 or 0.1%) are below a certain chosen limit
(here -2. standard deviations).
*/
  double z = -2.; // z to give prob p
  double p = 0.001; // only 0.1% below z = -2

  ex_out << "Normal distribution with mean = " << N01.location()  // aka N01.mean()
    << ", standard deviation " << N01.scale() // aka N01.standard_deviation()
    << ", has " << "fraction <= " << z
    << ", p = "  << cdf(N01, z) << endl;
  ex_out << "Normal distribution with mean = " << N01.location()
    << ", standard deviation " << N01.scale()
    << ", has " << "fraction > " << z
    << ", p = "  << cdf(complement(N01, z)) << endl; // Note: uses complement.
/*`
[pre
Normal distribution with mean = 0 has fraction <= -2, p = 0.0227501
Normal distribution with mean = 0 has fraction > -2, p = 0.97725
]
Noting that p = 0.02 instead of our target of 0.001,
we can now use `find_scale` to give a new standard deviation.
*/
   double l = N01.location();
   double s = find_scale<normal>(z, p, l);
   ex_out << "scale (standard deviation) = " << s << endl;
/*`
that outputs:
[pre
scale (standard deviation) = 0.647201
]
showing that we need to reduce the standard deviation from 1. to 0.65.

Then we can check that we have achieved our objective
by constructing a new distribution
with the new standard deviation (but same zero mean):
*/
  normal np001pc(N01.location(), s);
/*`
And re-calculating the fraction below (and above) our chosen limit.
*/
  ex_out << "Normal distribution with mean = " << l
    << " has " << "fraction <= " << z
    << ", p = "  << cdf(np001pc, z) << endl;
  ex_out << "Normal distribution with mean = " << l
    << " has " << "fraction > " << z
    << ", p = "  << cdf(complement(np001pc, z)) << endl;
/*`
[pre
Normal distribution with mean = 0 has fraction <= -2, p = 0.001
Normal distribution with mean = 0 has fraction > -2, p = 0.999
]

[h4 Controlling how Errors from find_scale are handled]
We can also control the policy for handling various errors.
For example, we can define a new (possibly unwise)
policy to ignore domain errors ('bad' arguments).

Unless we are using the boost::math namespace, we will need:
*/
  using boost::math::policies::policy;
  using boost::math::policies::domain_error;
  using boost::math::policies::ignore_error;

/*`
Using a typedef is convenient, especially if it is re-used,
although it is not required, as the various examples below show.
*/
  typedef policy<domain_error<ignore_error> > ignore_domain_policy;
  // find_scale with new policy, using typedef.
  l = find_scale<normal>(z, p, l, ignore_domain_policy());
  // Default policy policy<>, needs using boost::math::policies::policy;

  l = find_scale<normal>(z, p, l, policy<>());
  // Default policy, fully specified.
  l = find_scale<normal>(z, p, l, boost::math::policies::policy<>());
  // New policy, without typedef.
  l = find_scale<normal>(z, p, l, policy<domain_error<ignore_error> >());
/*`
If we want to express a probability, say 0.999, that is a complement, `1 - p`
we should not even think of writing `find_scale<normal>(z, 1 - p, l)`,
but use the __complements version (see __why_complements).
*/
  z = -2.;
  double q = 0.999; // = 1 - p; // complement of 0.001.
  sd = find_scale<normal>(complement(z, q, l));

  normal np95pc(l, sd); // Same standard_deviation (scale) but with mean(scale) shifted
  ex_out << "Normal distribution with mean = " << l << " has "
    << "fraction <= " << z << " = "  << cdf(np95pc, z) << endl;
  ex_out << "Normal distribution with mean = " << l << " has "
    << "fraction > " << z << " = "  << cdf(complement(np95pc, z)) << endl;

/*`
Sadly, it is all too easy to get probabilities the wrong way round,
when you may get a warning like this:
[pre
Message from thrown exception was:
   Error in function boost::math::find_scale<Dist, Policy>(complement(double, double, double, Policy)):
   Computed scale (-0.48043523852179076) is <= 0! Was the complement intended?
]
The default error handling policy is to throw an exception with this message,
but if you chose a policy to ignore the error,
the (impossible) negative scale is quietly returned.
*/
//] [/find_scale2]
  }
  catch(const std::exception& e)
  { // Always useful to include try & catch blocks because default policies
    // are to throw exceptions on arguments that cause errors like underflow, overflow.
    // Lacking try & catch blocks, the program will abort without a message below,
    // which may give some helpful clues as to the cause of the exception.
    ex_out <<
      "\n""Message from thrown exception was:\n   " << e.what() << std::endl;
  }
  return 0;
    return 0;  // main's implicit return
}  // int example_main()

//[find_scale_example_output
/*`
[pre
Example: Find scale (standard deviation).
Normal distribution with mean = 0, standard deviation 1, has fraction <= -2, p = 0.0227501
Normal distribution with mean = 0, standard deviation 1, has fraction > -2, p = 0.97725
scale (standard deviation) = 0.647201
Normal distribution with mean = 0 has fraction <= -2, p = 0.001
Normal distribution with mean = 0 has fraction > -2, p = 0.999
Normal distribution with mean = 0.946339 has fraction <= -2 = 0.001
Normal distribution with mean = 0.946339 has fraction > -2 = 0.999
]
*/
//] [/find_scale_example_output]

TEST_CASE("example: find_scale_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Example: Find scale (standard deviation).
Normal distribution with mean = 0, standard deviation 1, has fraction <= -2, p = 0.0227501
Normal distribution with mean = 0, standard deviation 1, has fraction > -2, p = 0.97725
scale (standard deviation) = 0.647201
Normal distribution with mean = 0 has fraction <= -2, p = 0.001
Normal distribution with mean = 0 has fraction > -2, p = 0.999
Normal distribution with mean = 0.946339 has fraction <= -2 = 0.001
Normal distribution with mean = 0.946339 has fraction > -2 = 0.999
)ex"));
}
}  // namespace ex_find_scale_example

// libs/math/example/gradient_descent_example.cpp
namespace ex_gradient_descent_example {
namespace rdiff = boost::math::differentiation::reverse_mode;
namespace bopt  = boost::math::optimization;
double random_double(double min = 0.0, double max = 1.0)
{
    static thread_local std::mt19937       rng{std::random_device{}()};
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}

template<typename S>
struct vec3
{
    /**
     * @brief R^3 coordinates of particle on Thomson Sphere
     */
    S x, y, z;
};

template<class S>
static inline vec3<S> sph_to_xyz(const S& theta, const S& phi)
{
    /**
     * convenience overload to convert from [theta,phi] -> x, y, z
     */
    return {sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)};
}

template<typename T>
T thomson_energy(std::vector<T>& r)
{
    /* inverse square law
     */
    const size_t N    = r.size() / 2;
    const T      tiny = T(1e-12);

    T E = 0;
    for (size_t i = 0; i < N; ++i) {
        const T& theta_i = r[2 * i + 0];
        const T& phi_i   = r[2 * i + 1];
        auto     ri      = sph_to_xyz(theta_i, phi_i);

        for (size_t j = i + 1; j < N; ++j) {
            const T& theta_j = r[2 * j + 0];
            const T& phi_j   = r[2 * j + 1];
            auto     rj      = sph_to_xyz(theta_j, phi_j);

            T dx = ri.x - rj.x;
            T dy = ri.y - rj.y;
            T dz = ri.z - rj.z;

            T d2 = dx * dx + dy * dy + dz * dz + tiny;
            E += 1.0 / sqrt(d2);
        }
    }
    return E;
}

template<class T>
std::vector<rdiff::rvar<T, 1>> init_theta_phi_uniform(size_t N, unsigned seed = 12345)
{
    const T pi = T(3.1415926535897932384626433832795);

    std::mt19937                      rng(seed);
    std::uniform_real_distribution<T> unif01(T(0), T(1));
    std::uniform_real_distribution<T> unifm11(T(-1), T(1));

    std::vector<rdiff::rvar<T, 1>> u;
    u.reserve(2 * N);

    for (size_t i = 0; i < N; ++i) {
        T z     = unifm11(rng);
        T phi   = (T(2) * pi) * unif01(rng) - pi;
        T theta = std::acos(z);

        u.emplace_back(theta);
        u.emplace_back(phi);
    }
    return u;
}

int example_main(int argc, char* argv[])
{
    /*
    if (argc != 2) {
        ex_out << "Usage: " << argv[0] << " <N>\n";
        return 1;
    }*/

    const int    N      = 2;// std::stoi(argv[1]);
    const int    NSTEPS = 100000;
    const double lr     = 1e-3;

    auto u_ad = init_theta_phi_uniform<double>(N);

    auto gdopt = bopt::make_gradient_descent(&thomson_energy<rdiff::rvar<double, 1>>, u_ad, lr);

    // filenames
    std::string pos_filename    = "thomson_" + std::to_string(N) + ".csv";
    std::string energy_filename = "energy_" + std::to_string(N) + ".csv";

    std::ofstream pos_out(pos_filename);
    std::ofstream energy_out(energy_filename);

    pos_out << "step,particle,x,y,z\n";
    energy_out << "step,energy\n";

    /*
    for (int step = 0; step < NSTEPS; ++step) {
        gdopt.step();
        for (int pi = 0; pi < N; ++pi) {
            double theta = u_ad[2 * pi + 0].item();
            double phi   = u_ad[2 * pi + 1].item();
            auto   r     = sph_to_xyz(theta, phi);
            pos_out << step << "," << pi << "," << r.x << "," << r.y << "," << r.z << "\n";
        }
        auto E = gdopt.objective_value();
        energy_out << step << "," << E << "\n";
    }*/

    auto result = minimize(gdopt);
    for (int pi = 0; pi < N; ++pi) {
        double theta = u_ad[2 * pi + 0].item();
        double phi   = u_ad[2 * pi + 1].item();
        auto   r     = sph_to_xyz(theta, phi);
        pos_out << pi << "," << r.x << "," << r.y << "," << r.z << "\n";
    }
    auto E = gdopt.objective_value();
    energy_out << "," << E << "\n";

    pos_out.close();
    energy_out.close();

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: gradient_descent_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_gradient_descent_example

// libs/math/example/inverse_chi_squared_bayes_eg.cpp
namespace ex_inverse_chi_squared_bayes_eg {
int example_main()
{
   using std::endl;

  using ::boost::math::inverse_chi_squared;
  using ::boost::math::inverse_gamma;
  using ::boost::math::quantile;
  using ::boost::math::cdf;
  
  ex_out << "Inverse_chi_squared_distribution Bayes example: " << endl <<endl;

  ex_out.precision(3);
// Examples of using the inverse_chi_squared distribution.

//[inverse_chi_squared_bayes_eg_1
/*`
The scaled-inversed-chi-squared distribution is the conjugate prior distribution
for the variance ([sigma][super 2]) parameter of a normal distribution
with known expectation ([mu]).
As such it has widespread application in Bayesian statistics:

In [@http://en.wikipedia.org/wiki/Bayesian_inference Bayesian inference],
the strength of belief into certain parameter values is
itself described through a distribution. Parameters
hence become themselves modelled and interpreted as random variables.

In this worked example, we perform such a Bayesian analysis by using
the scaled-inverse-chi-squared distribution as prior and posterior distribution
for the variance parameter of a normal distribution.

For more general information on Bayesian type of analyses,
see:

* Andrew Gelman, John B. Carlin, Hal E. Stern, Donald B. Rubin, Bayesian Data Analysis,
2003, ISBN 978-1439840955.

* Jim Albert, Bayesian Computation with R, Springer, 2009, ISBN 978-0387922973.

(As the scaled-inversed-chi-squared is another parameterization of the inverse-gamma distribution,
this example could also have used the inverse-gamma distribution).

Consider precision machines which produce balls for a high-quality ball bearing.
Ideally each ball should have a diameter of precisely 3000 [mu]m (3 mm).
Assume that machines generally produce balls of that size on average (mean),
but individual balls can vary slightly in either direction
following (approximately) a normal distribution. Depending on various production conditions
(e.g. raw material used for balls, workplace temperature and humidity, maintenance frequency and quality)
some machines produce balls tighter distributed around the target of 3000 [mu]m,
while others produce balls with a wider distribution. 
Therefore the variance parameter of the normal distribution of the ball sizes varies
from machine to machine. An extensive survey by the precision machinery manufacturer, however,
has shown that most machines operate with a variance between 15 and 50,
and near 25 [mu]m[super 2] on average.

Using this information, we want to model the variance of the machines.
The variance is strictly positive, and therefore we look for a statistical distribution
with support in the positive domain of the real numbers.
Given the expectation of the normal distribution of the balls is known (3000 [mu]m),
for reasons of conjugacy, it is customary practice in Bayesian statistics
to model the variance to be scaled-inverse-chi-squared distributed. 

In a first step, we will try to use the survey information to model
the general knowledge about the variance parameter of machines measured by the manufacturer. 
This will provide us with a generic prior distribution that is applicable
if nothing more specific is known about a particular machine.

In a second step, we will then combine the prior-distribution information in a Bayesian analysis
with data on a specific single machine to derive a posterior distribution for that machine.

[h5 Step one: Using the survey information.]

Using the survey results, we try to find the parameter set
of a scaled-inverse-chi-squared distribution 
so that the properties of this distribution match the results. 
Using the mathematical properties of the scaled-inverse-chi-squared distribution 
as guideline, we see that that both the mean and mode of the scaled-inverse-chi-squared distribution
are approximately given by the scale parameter (s) of the distribution. As the survey machines operated at a
variance of 25 [mu]m[super 2] on average, we hence set the scale parameter (s[sub prior]) of our prior distribution 
equal to this value. Using some trial-and-error and calls to the global quantile function, we also find that a
value of 20 for the degrees-of-freedom ([nu][sub prior]) parameter is adequate so that
most of the prior distribution mass is located between 15 and 50 (see figure below).

We first construct our prior distribution using these values, and then list out a few quantiles:

*/
  double priorDF = 20.0;
  double priorScale = 25.0; 

  inverse_chi_squared prior(priorDF, priorScale);
  // Using an inverse_gamma distribution instead, we could equivalently write
  // inverse_gamma prior(priorDF / 2.0, priorScale * priorDF / 2.0);
  
  ex_out << "Prior distribution:" << endl << endl;
  ex_out << "  2.5% quantile: " << quantile(prior, 0.025) << endl;
  ex_out << "  50% quantile: " << quantile(prior, 0.5) << endl;
  ex_out << "  97.5% quantile: " << quantile(prior, 0.975) << endl << endl;

 //] [/inverse_chi_squared_bayes_eg_1]

//[inverse_chi_squared_bayes_eg_output_1
/*`This produces this output:

    Prior distribution:
  
    2.5% quantile: 14.6
    50% quantile: 25.9
    97.5% quantile: 52.1

*/
//] [/inverse_chi_squared_bayes_eg_output_1]

//[inverse_chi_squared_bayes_eg_2
/*`
Based on this distribution, we can now calculate the probability of having a machine
working with an unusual work precision (variance) at <= 15 or > 50.
For this task, we use calls to the `boost::math::` functions `cdf` and `complement`,
respectively, and find a probability of about 0.031 (3.1%) for each case.
*/
  
  ex_out << "  probability variance <= 15: " << boost::math::cdf(prior, 15.0) << endl;
  ex_out << "  probability variance <= 25: " << boost::math::cdf(prior, 25.0) << endl;
  ex_out << "  probability variance > 50: " 
    << boost::math::cdf(boost::math::complement(prior, 50.0))
  << endl << endl;
 //] [/inverse_chi_squared_bayes_eg_2]

//[inverse_chi_squared_bayes_eg_output_2
/*`This produces this output:

    probability variance <= 15: 0.031
    probability variance <= 25: 0.458
    probability variance > 50: 0.0318

*/
//] [/inverse_chi_squared_bayes_eg_output_2]
  
//[inverse_chi_squared_bayes_eg_3
/*`Therefore, only 3.1% of all precision machines produce balls with a variance of 15 or less
(particularly precise machines),
but also only 3.2% of all machines produce balls
with a variance of as high as 50 or more (particularly imprecise machines). Moreover, slightly more than
one-half (1 - 0.458 = 54.2%) of the machines work at a variance greater than 25. 

Notice here the distinction between a
[@http://en.wikipedia.org/wiki/Bayesian_inference Bayesian] analysis and a
[@http://en.wikipedia.org/wiki/Frequentist_inference frequentist] analysis:
because we model the variance as random variable itself,
we can calculate and straightforwardly interpret probabilities for given parameter values directly,
while such an approach is not possible (and interpretationally a strict ['must-not]) in the frequentist
world.

[h5 Step 2: Investigate a single machine]

In the second step, we investigate a single machine,
which is suspected to suffer from a major fault
as the produced balls show fairly high size variability.
Based on the prior distribution of generic machinery performance (derived above)
and data on balls produced by the suspect machine, we calculate the posterior distribution for that 
machine and use its properties for guidance regarding continued machine operation or suspension.

It can be shown that if the prior distribution
was chosen to be scaled-inverse-chi-square distributed,
then the posterior distribution is also scaled-inverse-chi-squared-distributed 
(prior and posterior distributions are hence conjugate).
For more details regarding conjugacy and formula to derive the parameters set
for the posterior distribution see
[@http://en.wikipedia.org/wiki/Conjugate_prior Conjugate prior].


Given the prior distribution parameters and sample data (of size n), the posterior distribution parameters 
are given by the two expressions:

__spaces [nu][sub posterior] = [nu][sub prior] + n

which gives the posteriorDF below, and

__spaces s[sub posterior] = ([nu][sub prior]s[sub prior] + [Sigma][super n][sub i=1](x[sub i] - [mu])[super 2]) / ([nu][sub prior] + n)

which after some rearrangement gives the formula for the posteriorScale below.

Machine-specific data consist of 100 balls which were accurately measured
and show the expected mean of 3000 [mu]m and a sample variance of 55 (calculated for a sample mean defined to be 3000 exactly).
From these data, the prior parameterization, and noting that the term 
[Sigma][super n][sub i=1](x[sub i] - [mu])[super 2] equals the sample variance multiplied by n - 1,
it follows that the posterior distribution of the variance parameter
is scaled-inverse-chi-squared distribution with degrees-of-freedom ([nu][sub posterior]) = 120 and 
scale (s[sub posterior]) = 49.54.
*/

  int ballsSampleSize = 100;
  ex_out <<"balls sample size: " << ballsSampleSize << endl;
  double ballsSampleVariance = 55.0;
  ex_out <<"balls sample variance: " << ballsSampleVariance << endl;

  double posteriorDF = priorDF + ballsSampleSize;
  ex_out << "prior degrees-of-freedom: " << priorDF << endl;
  ex_out << "posterior degrees-of-freedom: " << posteriorDF << endl;
  
  double posteriorScale = 
    (priorDF * priorScale + (ballsSampleVariance * (ballsSampleSize - 1))) / posteriorDF;
  ex_out << "prior scale: " << priorScale  << endl;
  ex_out << "posterior scale: " << posteriorScale << endl;

/*`An interesting feature here is that one needs only to know a summary statistics of the sample
to parameterize the posterior distribution: the 100 individual ball measurements are irrelevant,
just knowledge of the sample variance and number of measurements is sufficient.
*/

//] [/inverse_chi_squared_bayes_eg_3]

//[inverse_chi_squared_bayes_eg_output_3
/*`That produces this output:


  balls sample size: 100
  balls sample variance: 55
  prior degrees-of-freedom: 20
  posterior degrees-of-freedom: 120
  prior scale: 25
  posterior scale: 49.5
   
  */
//] [/inverse_chi_squared_bayes_eg_output_3]

//[inverse_chi_squared_bayes_eg_4
/*`To compare the generic machinery performance with our suspect machine,
we calculate again the same quantiles and probabilities as above,
and find a distribution clearly shifted to greater values (see figure).

[graph prior_posterior_plot]

*/

 inverse_chi_squared posterior(posteriorDF, posteriorScale);

  ex_out << "Posterior distribution:" << endl << endl;
  ex_out << "  2.5% quantile: " << boost::math::quantile(posterior, 0.025) << endl;
  ex_out << "  50% quantile: " << boost::math::quantile(posterior, 0.5) << endl;
  ex_out << "  97.5% quantile: " << boost::math::quantile(posterior, 0.975) << endl << endl;

  ex_out << "  probability variance <= 15: " << boost::math::cdf(posterior, 15.0) << endl;
  ex_out << "  probability variance <= 25: " << boost::math::cdf(posterior, 25.0) << endl;
  ex_out << "  probability variance > 50: " 
    << boost::math::cdf(boost::math::complement(posterior, 50.0)) << endl;

//] [/inverse_chi_squared_bayes_eg_4]

//[inverse_chi_squared_bayes_eg_output_4
/*`This produces this output:

 Posterior distribution:
  
    2.5% quantile: 39.1
    50% quantile: 49.8
    97.5% quantile: 64.9
  
    probability variance <= 15: 2.97e-031
    probability variance <= 25: 8.85e-010
    probability variance > 50: 0.489
  
*/
//] [/inverse_chi_squared_bayes_eg_output_4]

//[inverse_chi_squared_bayes_eg_5
/*`Indeed, the probability that the machine works at a low variance (<= 15) is almost zero,
and even the probability of working at average or better performance is negligibly small
(less than one-millionth of a permille). 
On the other hand, with an almost near-half probability (49%), the machine operates in the
extreme high variance range of > 50 characteristic for poorly performing machines.

Based on this information the operation of the machine is taken out of use and serviced.

In summary, the Bayesian analysis allowed us to make exact probabilistic statements about a
parameter of interest, and hence provided us results with straightforward interpretation. 

*/
//] [/inverse_chi_squared_bayes_eg_5]

    return 0;  // main's implicit return
} // int example_main()

//[inverse_chi_squared_bayes_eg_output
/*`
[pre
 Inverse_chi_squared_distribution Bayes example: 
  
   Prior distribution:
  
    2.5% quantile: 14.6
    50% quantile: 25.9
    97.5% quantile: 52.1
  
    probability variance <= 15: 0.031
    probability variance <= 25: 0.458
    probability variance > 50: 0.0318
  
  balls sample size: 100
  balls sample variance: 55
  prior degrees-of-freedom: 20
  posterior degrees-of-freedom: 120
  prior scale: 25
  posterior scale: 49.5
  Posterior distribution:
  
    2.5% quantile: 39.1
    50% quantile: 49.8
    97.5% quantile: 64.9
  
    probability variance <= 15: 2.97e-031
    probability variance <= 25: 8.85e-010
    probability variance > 50: 0.489

] [/pre]
*/
//] [/inverse_chi_squared_bayes_eg_output]

TEST_CASE("example: inverse_chi_squared_bayes_eg") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Inverse_chi_squared_distribution Bayes example: 

Prior distribution:

  2.5% quantile: 14.6
  50% quantile: 25.9
  97.5% quantile: 52.1

  probability variance <= 15: 0.031
  probability variance <= 25: 0.458
  probability variance > 50: 0.0318

balls sample size: 100
balls sample variance: 55
prior degrees-of-freedom: 20
posterior degrees-of-freedom: 120
prior scale: 25
posterior scale: 49.5
Posterior distribution:

  2.5% quantile: 39.1
  50% quantile: 49.8
  97.5% quantile: 64.9

  probability variance <= 15: 2.97e-31
  probability variance <= 25: 8.85e-10
  probability variance > 50: 0.489
)ex"));
}
}  // namespace ex_inverse_chi_squared_bayes_eg

// libs/math/example/inverse_gamma_example.cpp
namespace ex_inverse_gamma_example {
using boost::math::gamma_p_inv; // Compute x given a
//using boost::math::gamma_q_inv;
//using boost::math::gamma_p_inva; // Compute a given x
//using boost::math::gamma_q_inva;

       using std::endl;
   using std::setprecision;
   using std::sqrt;

int example_main()
{
  ex_out << "Example 1 using Inverse Gamma function. " << endl;

  #ifdef BOOST_NO_CXX11_NUMERIC_LIMITS
  int max_digits10 = 2 + (boost::math::policies::digits<double, boost::math::policies::policy<> >() * 30103UL) / 100000UL;
  ex_out << "BOOST_NO_CXX11_NUMERIC_LIMITS is defined" << endl; 
#else 
  int max_digits10 = std::numeric_limits<double>::max_digits10;
#endif
  ex_out << "Show all potentially significant decimal digits std::numeric_limits<double>::max_digits10 = "
    << max_digits10 << endl; 
  ex_out.precision(max_digits10); // 

  double x = 1.;
  double a = 10;

  double r = boost::math::gamma_q_inv(a ,x);

  ex_out << " x = " << x << ", = gamma_q_inv(a,x)" << r << endl; // 
  
  return 0;
    return 0;  // main's implicit return
}  // int example_main()

/*

Output is:

*/

TEST_CASE("example: inverse_gamma_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Example 1 using Inverse Gamma function. 
Show all potentially significant decimal digits std::numeric_limits<double>::max_digits10 = 17
 x = 1, = gamma_q_inv(a,x)0
)ex"));
}
}  // namespace ex_inverse_gamma_example

// libs/math/example/lambert_w_diode.cpp
namespace ex_lambert_w_diode {
using boost::math::lambert_w0;

// 
// using std::endl;

/*!
Compute thermal voltage as a function of temperature,
about 25 mV at room temperature.
https://en.wikipedia.org/wiki/Boltzmann_constant#Role_in_semiconductor_physics:_the_thermal_voltage

\param temperature Temperature (degrees centigrade).
*/
const double v_thermal(double temperature)
{
  constexpr const double boltzmann_k = 1.38e-23; // joules/kelvin.
  const double charge_q = 1.6021766208e-19; // Charge of an electron (columb).
  double temp =+ 273; // Degrees C to K.
  return boltzmann_k * temp / charge_q;
} // v_thermal

/*!
Banwell & Jayakumar, equation 2
*/
double i(double isat, double vd, double vt, double nu)
{
  double i = isat * (exp(vd / (nu * vt)) - 1);
  return i;
} // 

/*!

Banwell & Jayakumar, Equation 4.
i current flow = isat
v voltage source.
isat reverse saturation current in equation 4.
(might implement equation 4 instead of simpler equation 5?).
vd voltage drop = v - i* rs  (equation 1).
vt  thermal voltage, 0.0257025 = 25 mV.
nu junction ideality factor (default = unity), also known as the emission coefficient.
re intrinsic emitter resistance, estimated to be 0.3 ohm from low current.
rsat reverse saturation current

\param v Voltage V to compute current I(V).
\param vt Thermal voltage, for example 0.0257025 = 25 mV, computed from boltzmann_k * temp / charge_q; 
\param rsat Resistance in series with the diode.
\param re Intrinsic emitter resistance (estimated to be 0.3 ohm from the Rs = 0 data)
\param isat Reverse saturation current (See equation 2).
\param nu Ideality factor (default = unity).

\returns I amp as function of V volt.

*/
double iv(double v, double vt, double rsat, double re, double isat, double nu = 1.)
{ 
  // V thermal 0.0257025 = 25 mV
  // was double i = (nu * vt/r) * lambert_w((i0 * r) / (nu * vt)); equ 5.

  rsat = rsat + re; 
  double i = nu * vt / rsat;
  ex_out << "nu * vt / rsat = " << i << std::endl; // 0.000103223

  double x = isat * rsat / (nu * vt);
  ex_out << "isat * rsat / (nu * vt) = " << x << std::endl;

  double eterm = (v + isat * rsat) / (nu * vt);
  ex_out << "(v + isat * rsat) / (nu * vt) = " << eterm << std::endl;

  double e = exp(eterm);
  ex_out << "exp(eterm) = " << e << std::endl;

  double w0 = lambert_w0(x * e);
  ex_out << "w0 = " << w0 << std::endl;
  return i * w0 - isat;

} // double iv

std::array<double, 5> rss = {0., 2.18, 10., 51., 249};  // series resistance (ohm).
std::array<double, 8> vds = { 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9 };  // Diode voltage.

int example_main()
{
  try
  {
    ex_out << "Lambert W diode current example." << std::endl;

    //[lambert_w_diode_example_1
    double x = 0.01;
    //ex_out << "Lambert W (" << x << ") = " << lambert_w(x) << std::endl; // 0.00990147

    double nu = 1.0;                  // Assumed ideal.
    double vt = v_thermal(25);        // v thermal, Shockley equation, expect about 25 mV at room temperature.
    double boltzmann_k = 1.38e-23;    // joules/kelvin
    double temp = 273 + 25;
    double charge_q = 1.6e-19;        // column
    vt = boltzmann_k * temp / charge_q;
    ex_out << "V thermal " 
       << vt << std::endl;            // V thermal 0.0257025 = 25 mV
    double rsat = 0.;
    double isat = 25.e-15;            //  25 fA;
    ex_out << "Isat = " << isat << std::endl;

    double re = 0.3;  // Estimated from slope of straight section of graph (equation 6).

    double v = 0.9;
    double icalc = iv(v, vt, 249., re, isat);

    ex_out << "voltage = " << v << ", current = " << icalc << ", " << log(icalc) << std::endl; // voltage = 0.9, current = 0.00108485, -6.82631
 //] [/lambert_w_diode_example_1]
  }
  catch (std::exception& ex)
  {
    ex_out << ex.what() << std::endl;
  }
    return 0;  // main's implicit return
}  // int example_main()

/*
   Output:
//[lambert_w_output_1
   Lambert W diode current example.
   V thermal 0.0257025
   Isat = 2.5e-14
   nu * vt / rsat = 0.000103099
   isat * rsat / (nu * vt) = 2.42486e-10
   (v + isat * rsat) / (nu * vt) = 35.016
   exp(eterm) = 1.61167e+15
   w0 = 10.5225
   voltage = 0.9, current = 0.00108485, -6.82631

//] [/lambert_w_output_1]
*/

TEST_CASE("example: lambert_w_diode") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Lambert W diode current example.
V thermal 0.0257025
Isat = 2.5e-14
nu * vt / rsat = 0.000103099
isat * rsat / (nu * vt) = 2.42486e-10
(v + isat * rsat) / (nu * vt) = 35.016
exp(eterm) = 1.61167e+15
w0 = 10.5225
voltage = 0.9, current = 0.00108485, -6.82631
)ex"));
}
}  // namespace ex_lambert_w_diode

// libs/math/example/legendre_stieltjes_example.cpp
namespace ex_legendre_stieltjes_example {
using boost::math::legendre_p;
using boost::math::legendre_p_zeros;
using boost::math::legendre_p_prime;
using boost::math::legendre_stieltjes;
using boost::multiprecision::cpp_bin_float_quad;
using boost::multiprecision::cpp_bin_float_100;
using boost::multiprecision::cpp_dec_float_100;

template<class Real>
void gauss_kronrod_rule(size_t order)
{
    ex_out << std::setprecision(std::numeric_limits<Real>::digits10);
    ex_out << std::fixed;
    auto gauss_nodes = boost::math::legendre_p_zeros<Real>(order);
    auto E = legendre_stieltjes<Real>(order + 1);
    std::vector<Real> gauss_weights(gauss_nodes.size(), std::numeric_limits<Real>::quiet_NaN());
    std::vector<Real> gauss_kronrod_weights(gauss_nodes.size(), std::numeric_limits<Real>::quiet_NaN());
    for (size_t i = 0; i < gauss_nodes.size(); ++i)
    {
        Real node = gauss_nodes[i];
        Real lp = legendre_p_prime<Real>(order, node);
        gauss_weights[i] = 2/( (1-node*node)*lp*lp);
        // P_n(x) = (2n)!/(2^n (n!)^2) pi_n(x), where pi_n is the monic Legendre polynomial.
        gauss_kronrod_weights[i] = gauss_weights[i] + static_cast<Real>(2)/(static_cast<Real>(order+1)*legendre_p_prime(order, node)*E(node));
    }

    ex_out << "static const std::vector<Real> gauss_nodes {\n";
    for (auto const & node : gauss_nodes)
    {
        ex_out << "    boost::lexical_cast<Real>(\"" << node << "\"),\n";
    }
    ex_out << "};\n\n";

    ex_out << "static const std::vector<Real> gauss_weights {\n";
    for (auto const & weight : gauss_weights)
    {
        ex_out << "    boost::lexical_cast<Real>(\"" << weight << "\"),\n";
    }
    ex_out << "};\n\n";

    ex_out << "static const std::vector<Real> gauss_kronrod_weights {\n";
    for (auto const & weight : gauss_kronrod_weights)
    {
        ex_out << "    boost::lexical_cast<Real>(\"" << weight << "\"),\n";
    }
    ex_out << "};\n\n";

    auto kronrod_nodes = E.zeros();
    std::vector<Real> kronrod_weights(kronrod_nodes.size());
    for (size_t i = 0; i < kronrod_weights.size(); ++i)
    {
        Real node = kronrod_nodes[i];
        kronrod_weights[i] = static_cast<Real>(2)/(static_cast<Real>(order+1)*legendre_p(order, node)*E.prime(node));
    }

    ex_out << "static const std::vector<Real> kronrod_nodes {\n";
    for (auto node : kronrod_nodes)
    {
        ex_out << "    boost::lexical_cast<Real>(\"" << node << "\"),\n";
    }
    ex_out << "};\n\n";

    ex_out << "static const std::vector<Real> kronrod_weights {\n";
    for (auto const & weight : kronrod_weights)
    {
        ex_out << "    boost::lexical_cast<Real>(\"" << weight << "\"),\n";
    }
    ex_out << "};\n\n";

}

int example_main()
{
    //ex_out << "Gauss-Kronrod 7-15 Rule:\n";
    //gauss_kronrod_rule<cpp_dec_float_100>(7);

    //ex_out << "\n\nGauss-Kronrod 10-21 Rule:\n";
    //gauss_kronrod_rule<cpp_dec_float_100>(10);

    ex_out << "\n\nGauss-Kronrod 15-31 Rule:\n";
    gauss_kronrod_rule<cpp_dec_float_100>(15);
    /*
    ex_out << "\n\nGauss-Kronrod 20-41 Rule:\n";
    gauss_kronrod_rule<cpp_dec_float_100>(20);

    ex_out << "\n\nGauss-Kronrod 25-51 Rule:\n";
    gauss_kronrod_rule<cpp_dec_float_100>(25);

    ex_out << "\n\nGauss-Kronrod 30-61 Rule:\n";
    gauss_kronrod_rule<cpp_dec_float_100>(30);

    ex_out << "\n\nGauss-Kronrod 35-71 Rule:\n";
    gauss_kronrod_rule<cpp_dec_float_100>(35);

    ex_out << "\n\nGauss-Kronrod 40-81 Rule:\n";
    gauss_kronrod_rule<cpp_dec_float_100>(40);*/
    return 0;  // main's implicit return
}

TEST_CASE("example: legendre_stieltjes_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(

Gauss-Kronrod 15-31 Rule:
static const std::vector<Real> gauss_nodes {
    boost::lexical_cast<Real>("0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"),
    boost::lexical_cast<Real>("0.2011940939974345223006283033945962078128364544626376796159497246099482390030201876018362580675210591"),
    boost::lexical_cast<Real>("0.3941513470775633698972073709810454683627527761586982550311653439516089577869614179754971141616597620"),
    boost::lexical_cast<Real>("0.5709721726085388475372267372539106412383863962827496048532654170541953798697585794834146285698261448"),
    boost::lexical_cast<Real>("0.7244177313601700474161860546139380096308992945841025635514234207041237816779252189961010976031343263"),
    boost::lexical_cast<Real>("0.8482065834104272162006483207742168513662561747369926340957275587606750751741454851976077197508214809"),
    boost::lexical_cast<Real>("0.9372733924007059043077589477102094712439962735153044579013630763502029737970455279505475861742680866"),
    boost::lexical_cast<Real>("0.9879925180204854284895657185866125811469728171237614899999975155873884373690194247127220503683191450"),
};

static const std::vector<Real> gauss_weights {
    boost::lexical_cast<Real>("0.2025782419255612728806201999675193148386621580094773567967041160514353987547460740933934407127880321"),
    boost::lexical_cast<Real>("0.1984314853271115764561183264438393248186925599575419934847379279291247975334342681333149991648178232"),
    boost::lexical_cast<Real>("0.1861610000155622110268005618664228245062260122779284028154957273100132555026991606189497688860993236"),
    boost::lexical_cast<Real>("0.1662692058169939335532008604812088111309001800984129073218651905635535632122785177107051742924155362"),
    boost::lexical_cast<Real>("0.1395706779261543144478047945110283225208502753155112432023911286310884445419078116807682573635713336"),
    boost::lexical_cast<Real>("0.1071592204671719350118695466858693034155437157581019806870223891218779948523157997256858571376086240"),
    boost::lexical_cast<Real>("0.0703660474881081247092674164506673384667080327543307198259072929143870555128742370448404520666939392"),
    boost::lexical_cast<Real>("0.0307532419961172683546283935772044177217481448334340742642282855042371894671171680390387707323994040"),
};

static const std::vector<Real> gauss_kronrod_weights {
    boost::lexical_cast<Real>("0.1013300070147915490173747927674925467709262725965962924673485837217410761577469666593241805068395675"),
    boost::lexical_cast<Real>("0.0991735987217919593323931734846031310595672608167132817348600956936515630643087457170566801282237907"),
    boost::lexical_cast<Real>("0.0931265981708253212254868727473457185619278813213173305602858791890520028745318550601149089904587167"),
    boost::lexical_cast<Real>("0.0830805028231330210382892472861037896015541882533687176072816048752336306438850560576307892283370889"),
    boost::lexical_cast<Real>("0.0698541213187282587095200770991474757860454351406715496987980931779926756249879988497486287785706675"),
    boost::lexical_cast<Real>("0.0534815246909280872653431472394302967715547609471167398132228887527274136162596254397148124751989875"),
    boost::lexical_cast<Real>("0.0353463607913758462220379484783600481226306789924208208681480233409025018372476809784346627242968101"),
    boost::lexical_cast<Real>("0.0150079473293161225383747630758072680946394364373876349792917597008964947461543343989617102274904025"),
};

static const std::vector<Real> kronrod_nodes {
    boost::lexical_cast<Real>("0.1011420669187174990270742314473923387874510574016418049580018950415109786245408305093132145154038100"),
    boost::lexical_cast<Real>("0.2991800071531688121667800242663889626616033827438208018412554573891808110251388446760232202015724356"),
    boost::lexical_cast<Real>("0.4850818636402396806936557402323506128663389308940731212936794360408023995516715597437184869084859528"),
    boost::lexical_cast<Real>("0.6509967412974169705337358953132746925469482260925996670896616057609330584104384079446039474722806037"),
    boost::lexical_cast<Real>("0.7904185014424659329676492948179473468621405199569761733236528064330830297463180705999473866422544553"),
    boost::lexical_cast<Real>("0.8972645323440819008825096564544958828317787114944278676397268760107853772147377122119539966191971612"),
    boost::lexical_cast<Real>("0.9677390756791391342573479787843372252833573373001316379746806222633580424945217480431938504820311851"),
    boost::lexical_cast<Real>("0.9980022986933970602851728401522712090734064423155572303483942797068334868283713456664897990776012528"),
};

static const std::vector<Real> kronrod_weights {
    boost::lexical_cast<Real>("0.1007698455238755950449466626175697219163483801353637306927892902948812276082276107747506018596540833"),
    boost::lexical_cast<Real>("0.0966427269836236785051799076275893351366565686304951989734076688829343923599628418265114025046645922"),
    boost::lexical_cast<Real>("0.0885644430562117706472754436937743032122667326906559678179960525748771445447498142607188375763251099"),
    boost::lexical_cast<Real>("0.0768496807577203788944327774826590067221091011679470005840890971124708210920340844182247315276902919"),
    boost::lexical_cast<Real>("0.0620095678006706402851392309608029321904000042103297235691478293956183762062723173330305842683038086"),
    boost::lexical_cast<Real>("0.0445897513247648766082272993732796902232566496679210965709808232118054507000599063664550364188971496"),
    boost::lexical_cast<Real>("0.0254608473267153201868740010196533593972717450468646405083779849824009034470091852676052057788197128"),
    boost::lexical_cast<Real>("0.0053774798729233489877920514301276498183080402431284197876486169536848635554354599213793172596490039"),
};

)ex"));
}
}  // namespace ex_legendre_stieltjes_example

// libs/math/example/nc_chi_sq_example.cpp
namespace ex_nc_chi_sq_example {
using boost::math::chi_squared;
using boost::math::non_central_chi_squared;

 using std::endl;
using std::setprecision;

int example_main()
{
   /*`
   Create a table of the power of the [chi][super 2] test at
   5% significance level, start with a table header:
   */
   ex_out << "[table\n[[[nu]]";
   for(int lam = 2; lam <= 20; lam += 2)
   {
      ex_out << "[[lambda]=" << lam << "]";
   }
   ex_out << "]\n";

   /*`
   (Note: the enclosing [] brackets are to format as a table in Boost.Quickbook).

   Enumerate the rows and columns and print the power of the test
   for each table cell:
   */

   for(int n = 2; n <= 20; ++n)
   {
      ex_out << "[[" << n << "]";
      for(int lam = 2; lam <= 20; lam += 2)
      {
         /*`
         Calculate the [chi][super 2] statistic for a 5% significance:
         */
         double cs = quantile(complement(chi_squared(n), 0.05));
         /*`
         The power of the test is given by the complement of the CDF
         of the non-central [chi][super 2] distribution:
         */
         double beta = cdf(complement(non_central_chi_squared(n, lam), cs));
         /*`
         Then output the cell value:
         */
         ex_out << "[" << setprecision(3) << beta << "]";
      }
      ex_out << "]" << endl;
   }
   ex_out << "]" << endl;
    return 0;  // main's implicit return
}

/*`
The output from this program is a table in Boost.Quickbook format as shown below.

We can interpret this as follows - for example if [nu]=10 and [lambda]=10
then the power of the test is 0.542 - so we have only a 54% chance of
correctly detecting that our null hypothesis is false, and a 46% chance
of incurring a type II error (failing to reject the null hypothesis when
it is in fact false):

[table
[[[nu]][[lambda]=2][[lambda]=4][[lambda]=6][[lambda]=8][[lambda]=10][[lambda]=12][[lambda]=14][[lambda]=16][[lambda]=18][[lambda]=20]]
[[2][0.226][0.415][0.584][0.718][0.815][0.883][0.928][0.957][0.974][0.985]]
[[3][0.192][0.359][0.518][0.654][0.761][0.84][0.896][0.934][0.959][0.975]]
[[4][0.171][0.32][0.47][0.605][0.716][0.802][0.866][0.912][0.943][0.964]]
[[5][0.157][0.292][0.433][0.564][0.677][0.769][0.839][0.89][0.927][0.952]]
[[6][0.146][0.27][0.403][0.531][0.644][0.738][0.813][0.869][0.911][0.94]]
[[7][0.138][0.252][0.378][0.502][0.614][0.71][0.788][0.849][0.895][0.928]]
[[8][0.131][0.238][0.357][0.477][0.588][0.685][0.765][0.829][0.879][0.915]]
[[9][0.125][0.225][0.339][0.454][0.564][0.661][0.744][0.811][0.863][0.903]]
[[10][0.121][0.215][0.323][0.435][0.542][0.64][0.723][0.793][0.848][0.891]]
[[11][0.117][0.206][0.309][0.417][0.523][0.62][0.704][0.775][0.833][0.878]]
[[12][0.113][0.198][0.297][0.402][0.505][0.601][0.686][0.759][0.818][0.866]]
[[13][0.11][0.191][0.286][0.387][0.488][0.584][0.669][0.743][0.804][0.854]]
[[14][0.108][0.185][0.276][0.374][0.473][0.567][0.653][0.728][0.791][0.842]]
[[15][0.105][0.179][0.267][0.362][0.459][0.552][0.638][0.713][0.777][0.83]]
[[16][0.103][0.174][0.259][0.351][0.446][0.538][0.623][0.699][0.764][0.819]]
[[17][0.101][0.169][0.251][0.341][0.434][0.525][0.609][0.686][0.752][0.807]]
[[18][0.0992][0.165][0.244][0.332][0.423][0.512][0.596][0.673][0.74][0.796]]
[[19][0.0976][0.161][0.238][0.323][0.412][0.5][0.584][0.66][0.728][0.786]]
[[20][0.0961][0.158][0.232][0.315][0.402][0.489][0.572][0.648][0.716][0.775]]
]

See [@../../example/nc_chi_sq_example.cpp nc_chi_sq_example.cpp] for the full C++ source code.

*/

//]

TEST_CASE("example: nc_chi_sq_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex([table
[[[nu]][[lambda]=2][[lambda]=4][[lambda]=6][[lambda]=8][[lambda]=10][[lambda]=12][[lambda]=14][[lambda]=16][[lambda]=18][[lambda]=20]]
[[2][0.226][0.415][0.584][0.718][0.815][0.883][0.928][0.957][0.974][0.985]]
[[3][0.192][0.359][0.518][0.654][0.761][0.84][0.896][0.934][0.959][0.975]]
[[4][0.171][0.32][0.47][0.605][0.716][0.802][0.866][0.912][0.943][0.964]]
[[5][0.157][0.292][0.433][0.564][0.677][0.769][0.839][0.89][0.927][0.952]]
[[6][0.146][0.27][0.403][0.531][0.644][0.738][0.813][0.869][0.911][0.94]]
[[7][0.138][0.252][0.378][0.502][0.614][0.71][0.788][0.849][0.895][0.928]]
[[8][0.131][0.238][0.357][0.477][0.588][0.685][0.765][0.829][0.879][0.915]]
[[9][0.125][0.225][0.339][0.454][0.564][0.661][0.744][0.811][0.863][0.903]]
[[10][0.121][0.215][0.323][0.435][0.542][0.64][0.723][0.793][0.848][0.891]]
[[11][0.117][0.206][0.309][0.417][0.523][0.62][0.704][0.775][0.833][0.878]]
[[12][0.113][0.198][0.297][0.402][0.505][0.601][0.686][0.759][0.818][0.866]]
[[13][0.11][0.191][0.286][0.387][0.488][0.584][0.669][0.743][0.804][0.854]]
[[14][0.108][0.185][0.276][0.374][0.473][0.567][0.653][0.728][0.791][0.842]]
[[15][0.105][0.179][0.267][0.362][0.459][0.552][0.638][0.713][0.777][0.83]]
[[16][0.103][0.174][0.259][0.351][0.446][0.538][0.623][0.699][0.764][0.819]]
[[17][0.101][0.169][0.251][0.341][0.434][0.525][0.609][0.686][0.752][0.807]]
[[18][0.0992][0.165][0.244][0.332][0.423][0.512][0.596][0.673][0.74][0.796]]
[[19][0.0976][0.161][0.238][0.323][0.412][0.5][0.584][0.66][0.728][0.786]]
[[20][0.0961][0.158][0.232][0.315][0.402][0.489][0.572][0.648][0.716][0.775]]
]
)ex"));
}
}  // namespace ex_nc_chi_sq_example

// libs/math/example/nesterov_example.cpp
namespace ex_nesterov_example {
namespace rdiff = boost::math::differentiation::reverse_mode;
namespace bopt  = boost::math::optimization;
double random_double(double min = 0.0, double max = 1.0)
{
    static thread_local std::mt19937       rng{std::random_device{}()};
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}

template<typename S>
struct vec3
{
    /**
     * @brief R^3 coordinates of particle on Thomson Sphere
     */
    S x, y, z;
};

template<class S>
static inline vec3<S> sph_to_xyz(const S& theta, const S& phi)
{
    /**
     * convenience overload to convert from [theta,phi] -> x, y, z
     */
    return {sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)};
}

template<typename T>
T thomson_energy(std::vector<T>& r)
{
    /* inverse square law
     */
    const size_t N    = r.size() / 2;
    const T      tiny = T(1e-12);

    T E = 0;
    for (size_t i = 0; i < N; ++i) {
        const T& theta_i = r[2 * i + 0];
        const T& phi_i   = r[2 * i + 1];
        auto     ri      = sph_to_xyz(theta_i, phi_i);

        for (size_t j = i + 1; j < N; ++j) {
            const T& theta_j = r[2 * j + 0];
            const T& phi_j   = r[2 * j + 1];
            auto     rj      = sph_to_xyz(theta_j, phi_j);

            T dx = ri.x - rj.x;
            T dy = ri.y - rj.y;
            T dz = ri.z - rj.z;

            T d2 = dx * dx + dy * dy + dz * dz + tiny;
            E += 1.0 / sqrt(d2);
        }
    }
    return E;
}

template<class T>
std::vector<rdiff::rvar<T, 1>> init_theta_phi_uniform(size_t N, unsigned seed = 12345)
{
    const T pi = T(3.1415926535897932384626433832795);

    std::mt19937                      rng(seed);
    std::uniform_real_distribution<T> unif01(T(0), T(1));
    std::uniform_real_distribution<T> unifm11(T(-1), T(1));

    std::vector<rdiff::rvar<T, 1>> u;
    u.reserve(2 * N);

    for (size_t i = 0; i < N; ++i) {
        T z     = unifm11(rng);
        T phi   = (T(2) * pi) * unif01(rng) - pi;
        T theta = std::acos(z);

        u.emplace_back(theta);
        u.emplace_back(phi);
    }
    return u;
}

int example_main(int argc, char* argv[])
{

    if (argc != 2) {
        ex_out << "Usage: " << argv[0] << " <N>\n";
        return 1;
    }

    const int    N      = std::stoi(argv[1]);
    const double lr     = 1e-3;
    const double mu     = 0.95;
    auto u_ad = init_theta_phi_uniform<double>(N);

    auto nesterov_opt = bopt::make_nag(&thomson_energy<rdiff::rvar<double, 1>>, u_ad, lr, mu);

    // filenames
    std::string pos_filename    = "thomson_" + std::to_string(N) + ".csv";
    std::string energy_filename = "nesterov_energy_" + std::to_string(N) + ".csv";

    std::ofstream pos_out(pos_filename);
    std::ofstream energy_out(energy_filename);

    energy_out << "step,energy\n";

    auto result = minimize(nesterov_opt);
    for (int pi = 0; pi < N; ++pi) {
        double theta = u_ad[2 * pi + 0].item();
        double phi   = u_ad[2 * pi + 1].item();
        auto   r     = sph_to_xyz(theta, phi);
        pos_out << pi << "," << r.x << "," << r.y << "," << r.z << "\n";
    }
    auto E = nesterov_opt.objective_value();
    int i = 0;
    for(auto& obj_hist : result.objective_history)
    {
        energy_out << i << "," << obj_hist << "\n";
        ++i;
    }
    energy_out << "," << E << "\n";

    pos_out.close();
    energy_out.close();

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: nesterov_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 1);
    CHECK(ex_out.str() == std::string_view(R"ex(Usage: example <N>
)ex"));
}
}  // namespace ex_nesterov_example

// libs/math/example/nonfinite_legacy.cpp
namespace ex_nonfinite_legacy {
using boost::math::nonfinite_num_put;
using boost::math::nonfinite_num_get;

using boost::math::legacy;


using std::endl;

using std::setfill;
using std::setw;

using std::locale;

using std::stringstream;
using std::numeric_limits;


int example_main()
{
  // Create a new locale with both the nonfinite facets.
  std::locale new_locale(std::locale(std::locale(),
    new boost::math::nonfinite_num_put<char>),
    new boost::math::nonfinite_num_get<char>);

  {
    stringstream ss;
    ss.imbue(new_locale);
    double inf = numeric_limits<double>::infinity();
    ss << inf; // Write out.
    double r;
    ss >> r; // Read back in.

    ex_out << "infinity output was " << inf << endl;
    ex_out << "infinity input was " << r << endl;

    BOOST_MATH_ASSERT(inf == r);
  }
  {
    stringstream ss;
    ss.imbue(new_locale);

    double nan = numeric_limits<double>::quiet_NaN();
    ss << nan; // Write out.
    double v;
    ss >> v; // Read back in.

    ex_out << "NaN output was " << nan << endl;
    ex_out << "NaN input was " << v << endl;

    // BOOST_MATH_ASSERT(nan == v); // Always fails because NaN == NaN fails!
    // BOOST_MATH_ASSERT(nan == numeric_limits<double>::quiet_NaN()); asserts!
  }

    return 0;  // main's implicit return
} // int example_main()

/*

Output:

infinity output was 1.#INF
infinity input was 1.#INF
NaN output was 1.#QNAN
NaN input was 1.#QNAN

*/

TEST_CASE("example: nonfinite_legacy") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(infinity output was inf
infinity input was inf
NaN output was nan
NaN input was nan
)ex"));
}
}  // namespace ex_nonfinite_legacy

// libs/math/example/nonfinite_num_facet_trap.cpp
namespace ex_nonfinite_num_facet_trap {
#ifdef _MSC_VER
#   pragma warning(disable : 4127) // conditional expression is constant.
// assumes C++ exceptions enabled /EHsc
#endif



using std::endl;
using std::hex;
using std::numeric_limits;

int example_main()
{
  using namespace boost::math;

  ex_out << "nonfinite_num_facet_trap.cpp" << std::endl;

  const double inf = +std::numeric_limits<double>::infinity ();
  const double nan = +std::numeric_limits<double>::quiet_NaN ();

  { // Output infinity and NaN with default flags (no trapping).
    std::ostringstream oss;
    std::locale default_locale (std::locale::classic ());
    std::locale C99_out_locale (default_locale, new boost::math::nonfinite_num_put<char>);
    oss.imbue (C99_out_locale);
    oss.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    oss << inf <<  ' ' << nan;
    ex_out << "oss.rdstate()  = " << hex << oss.rdstate() << endl; // 0
    ex_out << "os.str() = " << oss.str() << endl; // os.str() = inf nan
  }

  try
  { // // Output infinity with flags set to trap and catch any infinity.
    std::ostringstream oss;
    std::locale default_locale (std::locale::classic ());
    std::locale C99_out_locale (default_locale, new boost::math::nonfinite_num_put<char>(trap_infinity));
    oss.imbue (C99_out_locale);
    oss.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    // Note that which bit is set is implementation dependent, so enable exceptions for both.
    oss << inf;
    ex_out << "oss.rdstate()  = " << hex << oss.rdstate() << endl;
    ex_out << "oss.str() = " << oss.str() << endl;
  }
  catch(const std::ios_base::failure& e)
  { // Expect "Infinity".
    ex_out << "\n""Message from thrown exception was: " << e.what() << std::endl;
  }

  try
  { // // Output NaN with flags set to catch any NaNs.
    std::ostringstream oss;
    std::locale default_locale (std::locale::classic ());
    std::locale C99_out_locale (default_locale, new boost::math::nonfinite_num_put<char>(trap_nan));
    oss.imbue (C99_out_locale);
    oss.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    // Note that which bit is set is implementation dependent, so enable exceptions for both.
    oss << nan;
    ex_out << "oss.str() = " << oss.str() << endl;
  }
  catch(const std::ios_base::failure& e)
  { // Expect "Infinity".
    ex_out << "\n""Message from thrown exception was: " << e.what() << std::endl;
 }


  return 0; // end of nonfinite_num_facet_trap.cpp
    return 0;  // main's implicit return
} // int example_main()


/*

Output:

  nonfinite_num_facet_trap.cpp
  oss.rdstate()  = 0
  os.str() = inf nan

  Message from thrown exception was: Infinity

  Message from thrown exception was: NaN

*/

TEST_CASE("example: nonfinite_num_facet_trap") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(nonfinite_num_facet_trap.cpp
oss.rdstate()  = 0
os.str() = inf nan

Message from thrown exception was: Infinity: unspecified iostream_category error

Message from thrown exception was: NaN: unspecified iostream_category error
)ex"));
#endif
}
}  // namespace ex_nonfinite_num_facet_trap

// libs/math/example/normal_tables.cpp: its additions to namespace boost/std, outside the case namespace
namespace boost
{
namespace math
{
template <typename FPT>
int max_digits10()
{
// Since max_digits10 is not defined (or wrong) on older systems, define a local max_digits10.

  // Usage:   int m = max_digits10<boost::float64_t>();
  const int m =
#if (defined BOOST_NO_CXX11_NUMERIC_LIMITS) || (_MSC_VER == 1600) // is wrongly 8 not 9 for VS2010.
  2 + std::numeric_limits<FPT>::digits * 3010/10000;
#else
  std::numeric_limits<FPT>::max_digits10;
#endif
  return m;
}
} // namespace math
}
// libs/math/example/normal_tables.cpp
namespace ex_normal_tables {
// See Implementation of Float128 type, Overloading template functions with float128_t.

  using boost::math::normal; // typedef provides default type of double.

  // using std::endl;
  //using std::left; using std::showpoint; using std::noshowpoint;
  //using std::setw; using std::setprecision;
  //using std::numeric_limits;

  /*!
Function max_digits10
Returns maximum number of possibly significant decimal digits for a floating-point type FPT,
even for older compilers/standard libraries that
lack support for std::std::numeric_limits<FPT>::max_digits10,
when the Kahan formula 2 + binary_digits * 0.3010 is used instead.
Also provides the correct result for Visual Studio 2010 where the max_digits10 provided for float is wrong.
*/
 // namespace boost

template <typename FPT>
void normal_table()
{
  using namespace boost::math;

  FPT step = static_cast<FPT>(1.); // step in z.
  FPT range = static_cast<FPT>(10.); // min and max z = -range to +range.

  // Traditional tables are only computed to much lower precision.
  // but @c std::std::numeric_limits<double>::max_digits10;
  // on new Standard Libraries gives 17,
  // the maximum number of digits from 64-bit double that can possibly be significant.
  // @c std::std::numeric_limits<double>::digits10; == 15
  // is number of @b guaranteed digits, the other two digits being 'noisy'.
  // Here we use a custom version of max_digits10 which deals with those platforms
  // where @c std::numeric_limits is not specialized,
  // or @c std::numeric_limits<>::max_digits10 not implemented, or wrong.
  int precision = boost::math::max_digits10<FPT>();

// ex_out << typeid(FPT).name() << std::endl;
// demo_normal.cpp:85: undefined reference to `typeinfo for __float128'
// [@http://gcc.gnu.org/bugzilla/show_bug.cgi?id=43622   GCC 43622]
//  typeinfo for __float128 was missing GCC 4.9 Mar 2014, but OK for GCC 6.1.1.

   // Construct a standard normal distribution s, with
   // (default mean = zero, and standard deviation = unity)
   normal s;
   ex_out << "\nStandard normal distribution, mean = "<< s.mean()
      << ", standard deviation = " << s.standard_deviation() << std::endl;

  ex_out << "maxdigits_10 is " << precision
    << ", digits10 is " << std::numeric_limits<FPT>::digits10 << std::endl;

  ex_out << "Probability distribution function values" << std::endl;

  ex_out << "  z " "   PDF " << std::endl;
  for (FPT z = -range; z < range + step; z += step)
  {
    ex_out << std::left << std::setprecision(3) << std::setw(6) << z << " "
      << std::setprecision(precision) << std::setw(12) << pdf(s, z) << std::endl;
  }
  ex_out.precision(6); // Restore to default precision.

/*`And the area under the normal curve from -[infin] up to z,
  the cumulative distribution function (CDF).
*/
  // For a standard normal distribution:
  ex_out << "Standard normal mean = "<< s.mean()
    << ", standard deviation = " << s.standard_deviation() << std::endl;
  ex_out << "Integral (area under the curve) from - infinity up to z." << std::endl;
  ex_out << "  z " "   CDF " << std::endl;
  for (FPT z = -range; z < range + step; z += step)
  {
    ex_out << std::left << std::setprecision(3) << std::setw(6) << z << " "
      << std::setprecision(precision) << std::setw(12) << cdf(s, z) << std::endl;
  }
  ex_out.precision(6); // Reset to default precision.
} // template <typename FPT> void normal_table()

int example_main()
{
  ex_out << "\nExample: Normal distribution tables." << std::endl;

  using namespace boost::math;

  try
  {// Tip - always use try'n'catch blocks to ensure that messages from thrown exceptions are shown.

//[normal_table_1
#ifdef BOOST_FLOAT32_C
    normal_table<boost::float32_t>(); // Usually type float
#endif
    normal_table<boost::float64_t>(); // Usually type double. Assume that float64_t is always available.
#ifdef BOOST_FLOAT80_C
    normal_table<boost::float80_t>(); // Type long double on some X86 platforms.
#endif
#ifdef BOOST_FLOAT128_C
    normal_table<boost::float128_t>(); // Type _Quad on some Intel and __float128 on some GCC platforms.
#endif
    normal_table<boost::floatmax_t>();
//] [/normal_table_1 ]
  }
  catch(std::exception ex)
  {
    ex_out << "exception thrown " << ex.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}  // int example_main()


/*

GCC 4.8.1 with quadmath

Example: Normal distribution tables.

Standard normal distribution, mean = 0, standard deviation = 1
maxdigits_10 is 9, digits10 is 6
Probability distribution function values
  z    PDF
-10    7.69459863e-023
-9     1.02797736e-018
-8     5.05227108e-015
-7     9.13472041e-012
-6     6.07588285e-009
-5     1.48671951e-006
-4     0.000133830226
-3     0.00443184841
-2     0.0539909665
-1     0.241970725
0      0.39894228
1      0.241970725
2      0.0539909665
3      0.00443184841
4      0.000133830226
5      1.48671951e-006
6      6.07588285e-009
7      9.13472041e-012
8      5.05227108e-015
9      1.02797736e-018
10     7.69459863e-023
Standard normal mean = 0, standard deviation = 1
Integral (area under the curve) from - infinity up to z.
  z    CDF
-10    7.61985302e-024
-9     1.12858841e-019
-8     6.22096057e-016
-7     1.27981254e-012
-6     9.86587645e-010
-5     2.86651572e-007
-4     3.16712418e-005
-3     0.00134989803
-2     0.0227501319
-1     0.158655254
0      0.5
1      0.841344746
2      0.977249868
3      0.998650102
4      0.999968329
5      0.999999713
6      0.999999999
7      1
8      1
9      1
10     1

Standard normal distribution, mean = 0, standard deviation = 1
maxdigits_10 is 17, digits10 is 15
Probability distribution function values
  z    PDF
-10    7.6945986267064199e-023
-9     1.0279773571668917e-018
-8     5.0522710835368927e-015
-7     9.1347204083645953e-012
-6     6.0758828498232861e-009
-5     1.4867195147342979e-006
-4     0.00013383022576488537
-3     0.0044318484119380075
-2     0.053990966513188063
-1     0.24197072451914337
0      0.3989422804014327
1      0.24197072451914337
2      0.053990966513188063
3      0.0044318484119380075
4      0.00013383022576488537
5      1.4867195147342979e-006
6      6.0758828498232861e-009
7      9.1347204083645953e-012
8      5.0522710835368927e-015
9      1.0279773571668917e-018
10     7.6945986267064199e-023
Standard normal mean = 0, standard deviation = 1
Integral (area under the curve) from - infinity up to z.
  z    CDF
-10    7.6198530241605945e-024
-9     1.1285884059538422e-019
-8     6.2209605742718204e-016
-7     1.279812543885835e-012
-6     9.865876450377014e-010
-5     2.8665157187919455e-007
-4     3.1671241833119972e-005
-3     0.0013498980316300957
-2     0.022750131948179216
-1     0.15865525393145705
0      0.5
1      0.84134474606854293
2      0.97724986805182079
3      0.9986501019683699
4      0.99996832875816688
5      0.99999971334842808
6      0.9999999990134123
7      0.99999999999872013
8      0.99999999999999933
9      1
10     1

Standard normal distribution, mean = 0, standard deviation = 1
maxdigits_10 is 21, digits10 is 18
Probability distribution function values
  z    PDF
-10    7.69459862670641993759e-023
-9     1.0279773571668916523e-018
-8     5.05227108353689273243e-015
-7     9.13472040836459525705e-012
-6     6.07588284982328608733e-009
-5     1.48671951473429788965e-006
-4     0.00013383022576488536764
-3     0.00443184841193800752729
-2     0.0539909665131880628364
-1     0.241970724519143365328
0      0.398942280401432702863
1      0.241970724519143365328
2      0.0539909665131880628364
3      0.00443184841193800752729
4      0.00013383022576488536764
5      1.48671951473429788965e-006
6      6.07588284982328608733e-009
7      9.13472040836459525705e-012
8      5.05227108353689273243e-015
9      1.0279773571668916523e-018
10     7.69459862670641993759e-023
Standard normal mean = 0, standard deviation = 1
Integral (area under the curve) from - infinity up to z.
  z    CDF
-10    7.61985302416059451083e-024
-9     1.12858840595384222719e-019
-8     6.22096057427182035917e-016
-7     1.279812543885834962e-012
-6     9.86587645037701399241e-010
-5     2.86651571879194547129e-007
-4     3.16712418331199717608e-005
-3     0.00134989803163009566139
-2     0.0227501319481792155242
-1     0.158655253931457046468
0      0.5
1      0.841344746068542925777
2      0.977249868051820791415
3      0.998650101968369896532
4      0.999968328758166880021
5      0.999999713348428076465
6      0.999999999013412299576
7      0.999999999998720134897
8      0.999999999999999333866
9      1
10     1

Standard normal distribution, mean = 0, standard deviation = 1
maxdigits_10 is 36, digits10 is 34
Probability distribution function values
  z    PDF
-10    7.69459862670641993759264402330435296e-023
-9     1.02797735716689165230378750485667109e-018
-8     5.0522710835368927324337437844893081e-015
-7     9.13472040836459525705208369548147081e-012
-6     6.07588284982328608733411870229841611e-009
-5     1.48671951473429788965346931561839483e-006
-4     0.00013383022576488536764006964663309418
-3     0.00443184841193800752728870762098267733
-2     0.0539909665131880628363703067407186609
-1     0.241970724519143365327522587904240936
0      0.398942280401432702863218082711682655
1      0.241970724519143365327522587904240936
2      0.0539909665131880628363703067407186609
3      0.00443184841193800752728870762098267733
4      0.00013383022576488536764006964663309418
5      1.48671951473429788965346931561839483e-006
6      6.07588284982328608733411870229841611e-009
7      9.13472040836459525705208369548147081e-012
8      5.0522710835368927324337437844893081e-015
9      1.02797735716689165230378750485667109e-018
10     7.69459862670641993759264402330435296e-023
Standard normal mean = 0, standard deviation = 1
Integral (area under the curve) from - infinity up to z.
  z    CDF
-10    7.61985302416059451083278826816793623e-024
-9     1.1285884059538422271881384555435713e-019
-8     6.22096057427182035917417257601387863e-016
-7     1.27981254388583496200054074948511201e-012
-6     9.86587645037701399241244820583623953e-010
-5     2.86651571879194547128505464808623238e-007
-4     3.16712418331199717608064048146587766e-005
-3     0.001349898031630095661392854111682027
-2     0.0227501319481792155241528519127314212
-1     0.158655253931457046467912164189328905
0      0.5
1      0.841344746068542925776512220181757584
2      0.977249868051820791414741051994496956
3      0.998650101968369896532351503992686048
4      0.999968328758166880021462930017150939
5      0.999999713348428076464813329948810861
6      0.999999999013412299575520592043176293
7      0.999999999998720134897212119540199637
8      0.999999999999999333866185224906075746
9      1
10     1

Standard normal distribution, mean = 0, standard deviation = 1
maxdigits_10 is 36, digits10 is 34
Probability distribution function values
  z    PDF
-10    7.69459862670641993759264402330435296e-023
-9     1.02797735716689165230378750485667109e-018
-8     5.0522710835368927324337437844893081e-015
-7     9.13472040836459525705208369548147081e-012
-6     6.07588284982328608733411870229841611e-009
-5     1.48671951473429788965346931561839483e-006
-4     0.00013383022576488536764006964663309418
-3     0.00443184841193800752728870762098267733
-2     0.0539909665131880628363703067407186609
-1     0.241970724519143365327522587904240936
0      0.398942280401432702863218082711682655
1      0.241970724519143365327522587904240936
2      0.0539909665131880628363703067407186609
3      0.00443184841193800752728870762098267733
4      0.00013383022576488536764006964663309418
5      1.48671951473429788965346931561839483e-006
6      6.07588284982328608733411870229841611e-009
7      9.13472040836459525705208369548147081e-012
8      5.0522710835368927324337437844893081e-015
9      1.02797735716689165230378750485667109e-018
10     7.69459862670641993759264402330435296e-023
Standard normal mean = 0, standard deviation = 1
Integral (area under the curve) from - infinity up to z.
  z    CDF
-10    7.61985302416059451083278826816793623e-024
-9     1.1285884059538422271881384555435713e-019
-8     6.22096057427182035917417257601387863e-016
-7     1.27981254388583496200054074948511201e-012
-6     9.86587645037701399241244820583623953e-010
-5     2.86651571879194547128505464808623238e-007
-4     3.16712418331199717608064048146587766e-005
-3     0.001349898031630095661392854111682027
-2     0.0227501319481792155241528519127314212
-1     0.158655253931457046467912164189328905
0      0.5
1      0.841344746068542925776512220181757584
2      0.977249868051820791414741051994496956
3      0.998650101968369896532351503992686048
4      0.999968328758166880021462930017150939
5      0.999999713348428076464813329948810861
6      0.999999999013412299575520592043176293
7      0.999999999998720134897212119540199637
8      0.999999999999999333866185224906075746
9      1
10     1

MSVC 2013 64-bit
1>
1>  Example: Normal distribution tables.
1>
1>  Standard normal distribution, mean = 0, standard deviation = 1
1>  maxdigits_10 is 9, digits10 is 6
1>  Probability distribution function values
1>    z    PDF
1>  -10    7.69459863e-023
1>  -9     1.02797736e-018
1>  -8     5.05227108e-015
1>  -7     9.13472041e-012
1>  -6     6.07588285e-009
1>  -5     1.48671951e-006
1>  -4     0.000133830226
1>  -3     0.00443184841
1>  -2     0.0539909665
1>  -1     0.241970725
1>  0      0.39894228
1>  1      0.241970725
1>  2      0.0539909665
1>  3      0.00443184841
1>  4      0.000133830226
1>  5      1.48671951e-006
1>  6      6.07588285e-009
1>  7      9.13472041e-012
1>  8      5.05227108e-015
1>  9      1.02797736e-018
1>  10     7.69459863e-023
1>  Standard normal mean = 0, standard deviation = 1
1>  Integral (area under the curve) from - infinity up to z.
1>    z    CDF
1>  -10    7.61985302e-024
1>  -9     1.12858841e-019
1>  -8     6.22096057e-016
1>  -7     1.27981254e-012
1>  -6     9.86587645e-010
1>  -5     2.86651572e-007
1>  -4     3.16712418e-005
1>  -3     0.00134989803
1>  -2     0.0227501319
1>  -1     0.158655254
1>  0      0.5
1>  1      0.841344746
1>  2      0.977249868
1>  3      0.998650102
1>  4      0.999968329
1>  5      0.999999713
1>  6      0.999999999
1>  7      1
1>  8      1
1>  9      1
1>  10     1
1>
1>  Standard normal distribution, mean = 0, standard deviation = 1
1>  maxdigits_10 is 17, digits10 is 15
1>  Probability distribution function values
1>    z    PDF
1>  -10    7.6945986267064199e-023
1>  -9     1.0279773571668917e-018
1>  -8     5.0522710835368927e-015
1>  -7     9.1347204083645953e-012
1>  -6     6.0758828498232861e-009
1>  -5     1.4867195147342979e-006
1>  -4     0.00013383022576488537
1>  -3     0.0044318484119380075
1>  -2     0.053990966513188063
1>  -1     0.24197072451914337
1>  0      0.3989422804014327
1>  1      0.24197072451914337
1>  2      0.053990966513188063
1>  3      0.0044318484119380075
1>  4      0.00013383022576488537
1>  5      1.4867195147342979e-006
1>  6      6.0758828498232861e-009
1>  7      9.1347204083645953e-012
1>  8      5.0522710835368927e-015
1>  9      1.0279773571668917e-018
1>  10     7.6945986267064199e-023
1>  Standard normal mean = 0, standard deviation = 1
1>  Integral (area under the curve) from - infinity up to z.
1>    z    CDF
1>  -10    7.6198530241605813e-024
1>  -9     1.1285884059538408e-019
1>  -8     6.2209605742718292e-016
1>  -7     1.2798125438858352e-012
1>  -6     9.8658764503770161e-010
1>  -5     2.8665157187919439e-007
1>  -4     3.1671241833119979e-005
1>  -3     0.0013498980316300957
1>  -2     0.022750131948179219
1>  -1     0.15865525393145707
1>  0      0.5
1>  1      0.84134474606854293
1>  2      0.97724986805182079
1>  3      0.9986501019683699
1>  4      0.99996832875816688
1>  5      0.99999971334842808
1>  6      0.9999999990134123
1>  7      0.99999999999872013
1>  8      0.99999999999999933
1>  9      1
1>  10     1
1>
1>  Standard normal distribution, mean = 0, standard deviation = 1
1>  maxdigits_10 is 17, digits10 is 15
1>  Probability distribution function values
1>    z    PDF
1>  -10    7.6945986267064199e-023
1>  -9     1.0279773571668917e-018
1>  -8     5.0522710835368927e-015
1>  -7     9.1347204083645953e-012
1>  -6     6.0758828498232861e-009
1>  -5     1.4867195147342979e-006
1>  -4     0.00013383022576488537
1>  -3     0.0044318484119380075
1>  -2     0.053990966513188063
1>  -1     0.24197072451914337
1>  0      0.3989422804014327
1>  1      0.24197072451914337
1>  2      0.053990966513188063
1>  3      0.0044318484119380075
1>  4      0.00013383022576488537
1>  5      1.4867195147342979e-006
1>  6      6.0758828498232861e-009
1>  7      9.1347204083645953e-012
1>  8      5.0522710835368927e-015
1>  9      1.0279773571668917e-018
1>  10     7.6945986267064199e-023
1>  Standard normal mean = 0, standard deviation = 1
1>  Integral (area under the curve) from - infinity up to z.
1>    z    CDF
1>  -10    7.6198530241605813e-024
1>  -9     1.1285884059538408e-019
1>  -8     6.2209605742718292e-016
1>  -7     1.2798125438858352e-012
1>  -6     9.8658764503770161e-010
1>  -5     2.8665157187919439e-007
1>  -4     3.1671241833119979e-005
1>  -3     0.0013498980316300957
1>  -2     0.022750131948179219
1>  -1     0.15865525393145707
1>  0      0.5
1>  1      0.84134474606854293
1>  2      0.97724986805182079
1>  3      0.9986501019683699
1>  4      0.99996832875816688
1>  5      0.99999971334842808
1>  6      0.9999999990134123
1>  7      0.99999999999872013
1>  8      0.99999999999999933
1>  9      1
1>  10     1


*/

TEST_CASE("example: normal_tables") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(
Example: Normal distribution tables.

Standard normal distribution, mean = 0, standard deviation = 1
maxdigits_10 is 9, digits10 is 6
Probability distribution function values
  z    PDF 
-10    7.69459863e-23
-9     1.02797736e-18
-8     5.05227108e-15
-7     9.13472041e-12
-6     6.07588285e-09
-5     1.48671951e-06
-4     0.000133830226
-3     0.00443184841
-2     0.0539909665
-1     0.241970725 
0      0.39894228  
1      0.241970725 
2      0.0539909665
3      0.00443184841
4      0.000133830226
5      1.48671951e-06
6      6.07588285e-09
7      9.13472041e-12
8      5.05227108e-15
9      1.02797736e-18
10     7.69459863e-23
Standard normal mean = 0, standard deviation = 1
Integral (area under the curve) from - infinity up to z.
  z    CDF 
-10    7.61985302e-24
-9     1.12858841e-19
-8     6.22096057e-16
-7     1.27981254e-12
-6     9.86587645e-10
-5     2.86651572e-07
-4     3.16712418e-05
-3     0.00134989803
-2     0.0227501319
-1     0.158655254 
0      0.5         
1      0.841344746 
2      0.977249868 
3      0.998650102 
4      0.999968329 
5      0.999999713 
6      0.999999999 
7      1           
8      1           
9      1           
10     1           

Standard normal distribution, mean = 0, standard deviation = 1
maxdigits_10 is 17, digits10 is 15
Probability distribution function values
  z    PDF 
-10    7.6945986267064199e-23
-9     1.0279773571668917e-18
-8     5.0522710835368927e-15
-7     9.1347204083645953e-12
-6     6.0758828498232861e-09
-5     1.4867195147342979e-06
-4     0.00013383022576488537
-3     0.0044318484119380075
-2     0.053990966513188063
-1     0.24197072451914337
0      0.3989422804014327
1      0.24197072451914337
2      0.053990966513188063
3      0.0044318484119380075
4      0.00013383022576488537
5      1.4867195147342979e-06
6      6.0758828498232861e-09
7      9.1347204083645953e-12
8      5.0522710835368927e-15
9      1.0279773571668917e-18
10     7.6945986267064199e-23
Standard normal mean = 0, standard deviation = 1
Integral (area under the curve) from - infinity up to z.
  z    CDF 
-10    7.6198530241605945e-24
-9     1.1285884059538422e-19
-8     6.2209605742718204e-16
-7     1.279812543885835e-12
-6     9.865876450377014e-10
-5     2.8665157187919455e-07
-4     3.1671241833119972e-05
-3     0.0013498980316300957
-2     0.022750131948179216
-1     0.15865525393145705
0      0.5         
1      0.84134474606854293
2      0.97724986805182079
3      0.9986501019683699
4      0.99996832875816688
5      0.99999971334842808
6      0.9999999990134123
7      0.99999999999872013
8      0.99999999999999933
9      1           
10     1           

Standard normal distribution, mean = 0, standard deviation = 1
maxdigits_10 is 21, digits10 is 18
Probability distribution function values
  z    PDF 
-10    7.69459862670641993759e-23
-9     1.0279773571668916523e-18
-8     5.05227108353689273243e-15
-7     9.13472040836459525705e-12
-6     6.07588284982328608733e-09
-5     1.48671951473429788965e-06
-4     0.00013383022576488536764
-3     0.00443184841193800752729
-2     0.0539909665131880628364
-1     0.241970724519143365328
0      0.398942280401432702863
1      0.241970724519143365328
2      0.0539909665131880628364
3      0.00443184841193800752729
4      0.00013383022576488536764
5      1.48671951473429788965e-06
6      6.07588284982328608733e-09
7      9.13472040836459525705e-12
8      5.05227108353689273243e-15
9      1.0279773571668916523e-18
10     7.69459862670641993759e-23
Standard normal mean = 0, standard deviation = 1
Integral (area under the curve) from - infinity up to z.
  z    CDF 
-10    7.61985302416059451083e-24
-9     1.12858840595384222719e-19
-8     6.22096057427182035917e-16
-7     1.279812543885834962e-12
-6     9.86587645037701399241e-10
-5     2.86651571879194547129e-07
-4     3.16712418331199717608e-05
-3     0.00134989803163009566139
-2     0.0227501319481792155242
-1     0.158655253931457046468
0      0.5         
1      0.841344746068542925777
2      0.977249868051820791415
3      0.998650101968369896532
4      0.999968328758166880021
5      0.999999713348428076465
6      0.999999999013412299576
7      0.999999999998720134897
8      0.999999999999999333866
9      1           
10     1           

Standard normal distribution, mean = 0, standard deviation = 1
maxdigits_10 is 21, digits10 is 18
Probability distribution function values
  z    PDF 
-10    7.69459862670641993759e-23
-9     1.0279773571668916523e-18
-8     5.05227108353689273243e-15
-7     9.13472040836459525705e-12
-6     6.07588284982328608733e-09
-5     1.48671951473429788965e-06
-4     0.00013383022576488536764
-3     0.00443184841193800752729
-2     0.0539909665131880628364
-1     0.241970724519143365328
0      0.398942280401432702863
1      0.241970724519143365328
2      0.0539909665131880628364
3      0.00443184841193800752729
4      0.00013383022576488536764
5      1.48671951473429788965e-06
6      6.07588284982328608733e-09
7      9.13472040836459525705e-12
8      5.05227108353689273243e-15
9      1.0279773571668916523e-18
10     7.69459862670641993759e-23
Standard normal mean = 0, standard deviation = 1
Integral (area under the curve) from - infinity up to z.
  z    CDF 
-10    7.61985302416059451083e-24
-9     1.12858840595384222719e-19
-8     6.22096057427182035917e-16
-7     1.279812543885834962e-12
-6     9.86587645037701399241e-10
-5     2.86651571879194547129e-07
-4     3.16712418331199717608e-05
-3     0.00134989803163009566139
-2     0.0227501319481792155242
-1     0.158655253931457046468
0      0.5         
1      0.841344746068542925777
2      0.977249868051820791415
3      0.998650101968369896532
4      0.999968328758166880021
5      0.999999713348428076465
6      0.999999999013412299576
7      0.999999999998720134897
8      0.999999999999999333866
9      1           
10     1           
)ex"));
}
}  // namespace ex_normal_tables

// libs/math/example/ooura_fourier_integrals_multiprecision_example.cpp
namespace ex_ooura_fourier_integrals_multiprecision_example {
#if !defined(__cpp_structured_bindings) || (__cpp_structured_bindings < 201606L)
#  error "This example requires a C++17 compiler that supports 'structured bindings'. Try /std:c++17 or -std=c++17 or later."
#endif

//#define BOOST_MATH_INSTRUMENT_OOURA // or -DBOOST_MATH_INSTRUMENT_OOURA etc for diagnostic output.



int example_main()
{
  try
  {
    typedef boost::multiprecision::cpp_bin_float_quad Real;

    ex_out.precision(std::numeric_limits<Real>::max_digits10); // Show all potentially significant digits.

    using boost::math::quadrature::ooura_fourier_cos;
    using boost::math::constants::half_pi;
    using boost::math::constants::e;

   //[ooura_fourier_integrals_multiprecision_example_1

    // Use the default parameters for tolerance root_epsilon and eight levels for a type of 8 bytes.
    //auto integrator = ooura_fourier_cos<Real>();
    // Decide on a (tight) tolerance.
    const Real tol = 2 * std::numeric_limits<Real>::epsilon();
    auto integrator = ooura_fourier_cos<Real>(tol, 8); // Loops or gets worse for more than 8.

    auto f = [](Real x)
    { // More complex example function.
      return 1 / (x * x + 1);
    };

    double omega = 1;
    auto [result, relative_error] = integrator.integrate(f, omega);

    //] [/ooura_fourier_integrals_multiprecision_example_1]

    //[ooura_fourier_integrals_multiprecision_example_2
    ex_out << "Integral = " << result << ", relative error estimate " << relative_error << std::endl;

    const Real expected = half_pi<Real>() / e<Real>(); // Expect integral = 1/(2e)
    ex_out << "pi/(2e)  = " << expected << ", difference " << result - expected << std::endl;
    //] [/ooura_fourier_integrals_multiprecision_example_2]
  }
  catch (std::exception const & ex)
  {
    // Lacking try&catch blocks, the program will abort after any throw, whereas the
    // message below from the thrown exception will give some helpful clues as to the cause of the problem.
    ex_out << "\n""Message from thrown exception was:\n   " << ex.what() << std::endl;
  }
    return 0;  // main's implicit return
} // int example_main()

/*

//[ooura_fourier_integrals_example_multiprecision_output_1
``
Integral = 0.5778636748954608589550465916563501587, relative error estimate 4.609814684522163895264277312610830278e-17
pi/(2e) = 0.5778636748954608659545328919193707407, difference -6.999486300263020581921171645255733758e-18
``
//] [/ooura_fourier_integrals_example_multiprecision_output_1]


//[ooura_fourier_integrals_example_multiprecision_diagnostic_output_1
``
ooura_fourier_cos with relative error goal 3.851859888774471706111955885169854637e-34 & 15 levels.
epsilon for type = 1.925929944387235853055977942584927319e-34
h = 1.000000000000000000000000000000000, I_h = 0.588268622591776615359568690603776 = 0.5882686225917766153595686906037760, absolute error estimate = nan
h = 0.500000000000000000000000000000000, I_h = 0.577871642184837461311756940493259 = 0.5778716421848374613117569404932595, absolute error estimate = 1.039698040693915404781175011051656e-02
h = 0.250000000000000000000000000000000, I_h = 0.577863671186882539559996800783122 = 0.5778636711868825395599968007831220, absolute error estimate = 7.970997954921751760139710137450075e-06
h = 0.125000000000000000000000000000000, I_h = 0.577863674895460885593491133506723 = 0.5778636748954608855934911335067232, absolute error estimate = 3.708578346033494332723601147051768e-09
h = 0.062500000000000000000000000000000, I_h = 0.577863674895460858955046591656350 = 0.5778636748954608589550465916563502, absolute error estimate = 2.663844454185037302771663314961535e-17
h = 0.031250000000000000000000000000000, I_h = 0.577863674895460858955046591656348 = 0.5778636748954608589550465916563484, absolute error estimate = 1.733336949948512267750380148326435e-33
h = 0.015625000000000000000000000000000, I_h = 0.577863674895460858955046591656348 = 0.5778636748954608589550465916563479, absolute error estimate = 4.814824860968089632639944856462318e-34
h = 0.007812500000000000000000000000000, I_h = 0.577863674895460858955046591656347 = 0.5778636748954608589550465916563473, absolute error estimate = 6.740754805355325485695922799047246e-34
h = 0.003906250000000000000000000000000, I_h = 0.577863674895460858955046591656347 = 0.5778636748954608589550465916563475, absolute error estimate = 1.925929944387235853055977942584927e-34
Integral = 5.778636748954608589550465916563475e-01, relative error estimate 3.332844800697411177051445985473052e-34
pi/(2e)  = 5.778636748954608589550465916563481e-01, difference -6.740754805355325485695922799047246e-34
``
//] [/ooura_fourier_integrals_example_multiprecision_diagnostic_output_1]

*/

TEST_CASE("example: ooura_fourier_integrals_multiprecision_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Integral = 0.577863674895460858955046591656347462, relative error estimate 3.33284480069741117705144598547305187e-34
pi/(2e)  = 0.577863674895460858955046591656348137, difference -6.74075480535532548569592279904724561e-34
)ex"));
}
}  // namespace ex_ooura_fourier_integrals_multiprecision_example

// libs/math/example/policy_eg_2.cpp
namespace ex_policy_eg_2 {
 using std::endl;

//[policy_eg_2

using boost::math::tgamma;

int example_main()
{
   // using namespace boost::math::policies; // or
   using boost::math::policies::errno_on_error;
   using boost::math::policies::make_policy;
   using boost::math::policies::pole_error;
   using boost::math::policies::domain_error;
   using boost::math::policies::overflow_error;
   using boost::math::policies::evaluation_error;

   errno = 0;
   ex_out << "Result of tgamma(30000) is: " 
      << boost::math::tgamma(
         30000, 
         make_policy(
            domain_error<errno_on_error>(),
            pole_error<errno_on_error>(),
            overflow_error<errno_on_error>(),
            evaluation_error<errno_on_error>() 
         )
      ) << std::endl;
   // Check errno was set:
   ex_out << "errno = " << errno << std::endl;
   // and again with evaluation at a pole:
   ex_out << "Result of tgamma(-10) is: " 
      << boost::math::tgamma(
         -10, 
         make_policy(
            domain_error<errno_on_error>(),
            pole_error<errno_on_error>(),
            overflow_error<errno_on_error>(),
            evaluation_error<errno_on_error>() 
         )
      ) << std::endl;
   // Check errno was set:
   ex_out << "errno = " << errno << std::endl;
    return 0;  // main's implicit return
}

//] //[/policy_eg_2]

/*

Output:

  Result of tgamma(30000) is: 1.#INF
  errno = 34
  Result of tgamma(-10) is: 1.#QNAN
  errno = 33
*/

TEST_CASE("example: policy_eg_2") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Result of tgamma(30000) is: inf
errno = 34
Result of tgamma(-10) is: nan
errno = 33
)ex"));
}
}  // namespace ex_policy_eg_2

// libs/math/example/policy_eg_6.cpp
namespace ex_policy_eg_6 {
  using std::endl;

//[policy_eg_6

/*`
Suppose we want a set of distributions to behave as follows:

* Return infinity on overflow, rather than throwing an exception.
* Don't perform any promotion from double to long double internally.
* Return the closest integer result from the quantiles of discrete
distributions.

We'll begin by including the needed header for all the distributions:
*/


/*`

Open up an appropriate namespace, calling it `my_distributions`,
for our distributions, and define the policy type we want.
Any policies we don't specify here will inherit the defaults:

*/

namespace my_distributions
{
  using namespace boost::math::policies;
  // using boost::math::policies::errno_on_error; // etc.

  typedef policy<
     // return infinity and set errno rather than throw:
     overflow_error<errno_on_error>,
     // Don't promote double -> long double internally:
     promote_double<false>,
     // Return the closest integer result for discrete quantiles:
     discrete_quantile<integer_round_nearest>
  > my_policy;

/*`

All we need do now is invoke the BOOST_MATH_DECLARE_DISTRIBUTIONS
macro passing the floating point type `double` and policy types `my_policy` as arguments:

*/

BOOST_MATH_DECLARE_DISTRIBUTIONS(double, my_policy)

} // close namespace my_namespace

/*`

We now have a set of typedefs defined in namespace my_distributions
that all look something like this:

``
typedef boost::math::normal_distribution<double, my_policy> normal;
typedef boost::math::cauchy_distribution<double, my_policy> cauchy;
typedef boost::math::gamma_distribution<double, my_policy> gamma;
// etc
``

So that when we use my_distributions::normal we really end up using
`boost::math::normal_distribution<double, my_policy>`:

*/

int example_main()
{
   // Construct distribution with something we know will overflow
  // (using double rather than if promoted to long double):
   my_distributions::normal norm(10, 2);

   errno = 0;
   ex_out << "Result of quantile(norm, 0) is: "
      << quantile(norm, 0) << endl; // -infinity.
   ex_out << "errno = " << errno << endl;
   errno = 0;
   ex_out << "Result of quantile(norm, 1) is: "
      << quantile(norm, 1) << endl; // +infinity.
   ex_out << "errno = " << errno << endl;

   // Now try a discrete distribution.
   my_distributions::binomial binom(20, 0.25);
   ex_out << "Result of quantile(binom, 0.05) is: "
      << quantile(binom, 0.05) << endl; // To check we get integer results.
   ex_out << "Result of quantile(complement(binom, 0.05)) is: "
      << quantile(complement(binom, 0.05)) << endl;
    return 0;  // main's implicit return
}

/*`

Which outputs:

[pre
Result of quantile(norm, 0) is: -1.#INF
errno = 34
Result of quantile(norm, 1) is: 1.#INF
errno = 34
Result of quantile(binom, 0.05) is: 1
Result of quantile(complement(binom, 0.05)) is: 8
]

This mechanism is particularly useful when we want to define a
project-wide policy, and don't want to modify the Boost source
or set  project wide build macros (possibly fragile and easy to forget).

*/
//] //[/policy_eg_6]

TEST_CASE("example: policy_eg_6") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Result of quantile(norm, 0) is: -inf
errno = 34
Result of quantile(norm, 1) is: inf
errno = 34
Result of quantile(binom, 0.05) is: 1
Result of quantile(complement(binom, 0.05)) is: 8
)ex"));
}
}  // namespace ex_policy_eg_6

// libs/math/example/policy_ref_snip1.cpp
namespace ex_policy_ref_snip1 {
using boost::math::tgamma;

//using namespace boost::math::policies; may also be convenient.
using boost::math::policies::policy;
using boost::math::policies::evaluation_error;
using boost::math::policies::domain_error;
using boost::math::policies::overflow_error;
using boost::math::policies::domain_error;
using boost::math::policies::pole_error;
using boost::math::policies::errno_on_error;

// Define a policy:
typedef policy<
  domain_error<errno_on_error>, 
  pole_error<errno_on_error>,
  overflow_error<errno_on_error>,
  evaluation_error<errno_on_error> 
> my_policy;

double my_value = 0.; // 

// Call the function applying my_policy:
double t1 = tgamma(my_value, my_policy());

// Alternatively (and equivalently) we could use helpful function
// make_policy and define everything at the call site:
double t2 = tgamma(my_value,
  make_policy(
    domain_error<errno_on_error>(), 
    pole_error<errno_on_error>(),
    overflow_error<errno_on_error>(),
    evaluation_error<errno_on_error>() )
  );
//]

 using std::endl;

int example_main()
{
  ex_out << "my_value = " << my_value << endl;
  try
  { // First with default policy - throw an exception.
     ex_out << "tgamma(my_value) = " << tgamma(my_value) << endl; 
  }
  catch(const std::exception& e)
  {
     ex_out <<"\n""Message from thrown exception was:\n   " << e.what() << endl;
  }

  ex_out << "tgamma(my_value, my_policy() = " << t1 << endl;
  ex_out << "tgamma(my_value, make_policy(domain_error<errno_on_error>(), pole_error<errno_on_error>(), overflow_error<errno_on_error>(), evaluation_error<errno_on_error>() ) = " << t2 << endl;
    return 0;  // main's implicit return
}

/*
Output:
  my_value = 0
  
  Message from thrown exception was:
     Error in function boost::math::tgamma<long double>(long double): Evaluation of tgamma at a negative integer 0.
  tgamma(my_value, my_policy() = 1.#QNAN
  tgamma(my_value, make_policy(domain_error<errno_on_error>(), pole_error<errno_on_error>(), overflow_error<errno_on_error>(), evaluation_error<errno_on_error>() ) = 1.#QNAN
*/

TEST_CASE("example: policy_ref_snip1") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(my_value = 0
tgamma(my_value) = 
Message from thrown exception was:
   Error in function boost::math::tgamma<long double>(long double): Evaluation of tgamma at a negative integer 0.
tgamma(my_value, my_policy() = nan
tgamma(my_value, make_policy(domain_error<errno_on_error>(), pole_error<errno_on_error>(), overflow_error<errno_on_error>(), evaluation_error<errno_on_error>() ) = nan
)ex"));
}
}  // namespace ex_policy_ref_snip1

// libs/math/example/policy_ref_snip13.cpp
namespace ex_policy_ref_snip13 {
#ifdef _MSC_VER
#  pragma warning (disable : 4189) //  'd' : local variable is initialized but not referenced
#endif
#ifdef BOOST_GCC
#  pragma GCC diagnostic ignored "-Wunused-variable"
#endif

 using std::endl;

using std::domain_error;

//[policy_ref_snip13


namespace myspace
{ // using namespace boost::math::policies; // May be convenient in myspace.

  // Define a policy called my_policy to use.
  using boost::math::policies::policy;
  
// In this case we want all the distribution accessor functions to compile,
// even if they are mathematically undefined, so
// make the policy assert_undefined.
  using boost::math::policies::assert_undefined;

typedef policy<assert_undefined<false> > my_policy;

// Finally apply this policy to type double.
BOOST_MATH_DECLARE_DISTRIBUTIONS(double, my_policy)
} // namespace myspace

// Now we can use myspace::cauchy etc, which will use policy
// myspace::mypolicy:
//
// This compiles but throws a domain error exception at runtime.
// Caution! If you omit the try'n'catch blocks, 
// it will just silently terminate, giving no clues as to why! 
// So try'n'catch blocks are very strongly recommended.

void test_cauchy()
{
   try
   {
      double d = mean(myspace::cauchy());  // Cauchy does not have a mean!
      (void) d;
   }
   catch(const std::domain_error& e)
   {
      ex_out << e.what() << endl;
   }
}

//] //[/policy_ref_snip13]

int example_main()
{
   test_cauchy();
    return 0;  // main's implicit return
}

/*

Output:

policy_snip_13.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Release\policy_snip_13.exe
  Error in function boost::math::mean(cauchy<double>&): The Cauchy distribution does not have a mean: the only possible return value is 1.#QNAN.

  */

TEST_CASE("example: policy_ref_snip13") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Error in function boost::math::mean(cauchy<double>&): The Cauchy distribution does not have a mean: the only possible return value is nan.
)ex"));
}
}  // namespace ex_policy_ref_snip13

// libs/math/example/policy_ref_snip5.cpp
namespace ex_policy_ref_snip5 {
using boost::math::negative_binomial_distribution;

using namespace boost::math::policies;

typedef negative_binomial_distribution<
      double, 
      policy<discrete_quantile<real> > 
   > dist_type;
   
// Lower 5% quantile:
double x = quantile(dist_type(20, 0.3), 0.05);
// Upper 95% quantile:
double y = quantile(complement(dist_type(20, 0.3), 0.05));

//] //[/policy_ref_snip5]

 using std::endl;

int example_main()
{
  ex_out << "quantile(dist_type(20, 0.3), 0.05)  = " << x 
    << "\nquantile(complement(dist_type(20, 0.3), 0.05) = " << y << endl;
    return 0;  // main's implicit return
}

/*

Output:
  quantile(dist_type(20, 0.3), 0.05)  = 27.3898
  quantile(complement(dist_type(20, 0.3), 0.05) = 68.1584

  */

TEST_CASE("example: policy_ref_snip5") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(quantile(dist_type(20, 0.3), 0.05)  = 27.3898
quantile(complement(dist_type(20, 0.3), 0.05) = 68.1584
)ex"));
}
}  // namespace ex_policy_ref_snip5

// libs/math/example/policy_ref_snip9.cpp
namespace ex_policy_ref_snip9 {
 using std::endl;

//[policy_ref_snip9

using boost::math::tgamma;
using boost::math::policies::policy;
using boost::math::policies::digits10;

typedef policy<digits10<5> > my_pol_5; // Define a new, non-default, policy
// to calculate tgamma to accuracy of approximately 5 decimal digits.
//]

int example_main()
{
  ex_out.precision(5); // To only show 5 (hopefully) accurate decimal digits.
  double t = tgamma(12, my_pol_5()); // Apply the 5 decimal digits accuracy policy to use of tgamma.
  ex_out << "tgamma(12, my_pol_5() = " << t << endl;
    return 0;  // main's implicit return
}

/*

Output:
     tgamma(12, my_pol_5() = 3.9917e+007
*/

TEST_CASE("example: policy_ref_snip9") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(tgamma(12, my_pol_5() = 3.9917e+07
)ex"));
}
}  // namespace ex_policy_ref_snip9

// libs/math/example/root_finding_example.cpp
namespace ex_root_finding_example {
//[root_finding_include_1

//using boost::math::policies::policy;
//using boost::math::tools::newton_raphson_iterate;
//using boost::math::tools::halley_iterate; //
//using boost::math::tools::eps_tolerance; // Binary functor for specified number of bits.
//using boost::math::tools::bracket_and_solve_root;
//using boost::math::tools::toms748_solve;


//] [/root_finding_include_1]

// using boost::math::tuple;
// using boost::math::make_tuple;
// using boost::math::tie;
// which provide convenient aliases for various implementations,
// including std::tr1, depending on what is available.

// using std::endl;
//using std::setw; using std::setprecision;
//using std::numeric_limits;

/*

Let's suppose we want to find the root of a number ['a], and to start, compute the cube root.

So the equation we want to solve is:

__spaces ['f](x) = x[cubed] - a

We will first solve this without using any information
about the slope or curvature of the cube root function.

We then show how adding what we can know about this function, first just the slope,
the 1st derivation /f'(x)/, will speed homing in on the solution.

Lastly we show how adding the curvature /f''(x)/ too will speed convergence even more.

*/

//[root_finding_noderiv_1

template <class T>
struct cbrt_functor_noderiv
{ 
  //  cube root of x using only function - no derivatives.
  cbrt_functor_noderiv(T const& to_find_root_of) : a(to_find_root_of)
  { /* Constructor just stores value a to find root of. */ }
  T operator()(T const& x)
  {
    T fx = x*x*x - a; // Difference (estimate x^3 - a).
    return fx;
  }
private:
  T a; // to be 'cube_rooted'.
};
//] [/root_finding_noderiv_1

/*
Implementing the cube root function itself is fairly trivial now:
the hardest part is finding a good approximation to begin with.
In this case we'll just divide the exponent by three.
(There are better but more complex guess algorithms used in 'real-life'.)

Cube root function is 'Really Well Behaved' in that it is monotonic
and has only one root (we leave negative values 'as an exercise for the student').
*/

//[root_finding_noderiv_2

template <class T>
T cbrt_noderiv(T x)
{ 
  // return cube root of x using bracket_and_solve (no derivatives).
  using namespace std;                          // Help ADL of std functions.
  using namespace boost::math::tools;           // For bracket_and_solve_root.

  int exponent;
  frexp(x, &exponent);                          // Get exponent of z (ignore mantissa).
  T guess = ldexp(1., exponent/3);              // Rough guess is to divide the exponent by three.
  T factor = 2;                                 // How big steps to take when searching.

  const std::uintmax_t maxit = 20;            // Limit to maximum iterations.
  std::uintmax_t it = maxit;                  // Initially our chosen max iterations, but updated with actual.
  bool is_rising = true;                        // So if result if guess^3 is too low, then try increasing guess.
  int digits = std::numeric_limits<T>::digits;  // Maximum possible binary digits accuracy for type T.
  // Some fraction of digits is used to control how accurate to try to make the result.
  int get_digits = digits - 3;                  // We have to have a non-zero interval at each step, so
                                                // maximum accuracy is digits - 1.  But we also have to
                                                // allow for inaccuracy in f(x), otherwise the last few
                                                // iterations just thrash around.
  eps_tolerance<T> tol(get_digits);             // Set the tolerance.
  std::pair<T, T> r = bracket_and_solve_root(cbrt_functor_noderiv<T>(x), guess, factor, is_rising, tol, it);
  return r.first + (r.second - r.first)/2;      // Midway between brackets is our result, if necessary we could
                                                // return the result as an interval here.
}

/*`

[note The final parameter specifying a maximum number of iterations is optional.
However, it defaults to `std::uintmax_t maxit = (std::numeric_limits<std::uintmax_t>::max)();`
which is `18446744073709551615` and is more than anyone would wish to wait for!

So it may be wise to chose some reasonable estimate of how many iterations may be needed, 
In this case the function is so well behaved that we can chose a low value of 20.

Internally when Boost.Math uses these functions, it sets the maximum iterations to
`policies::get_max_root_iterations<Policy>();`.]

Should we have wished we can show how many iterations were used in `bracket_and_solve_root` 
(this information is lost outside `cbrt_noderiv`), for example with:

  if (it >= maxit)
  {
    ex_out << "Unable to locate solution in " << maxit << " iterations:"
      " Current best guess is between " << r.first << " and " << r.second << std::endl;
  }
  else
  {
    ex_out << "Converged after " << it << " (from maximum of " << maxit << " iterations)." << std::endl;
  }

for output like

  Converged after 11 (from maximum of 20 iterations).
*/
//] [/root_finding_noderiv_2]


// Cube root with 1st derivative (slope)

/*
We now solve the same problem, but using more information about the function,
to show how this can speed up finding the best estimate of the root.

For the root function, the 1st differential (the slope of the tangent to a curve at any point) is known.

If you need some reminders then
[@http://en.wikipedia.org/wiki/Derivative#Derivatives_of_elementary_functions Derivatives of elementary functions]
may help.

Using the rule that the derivative of ['x[super n]] for positive n (actually all nonzero n) is ['n x[super n-1]],
allows us to get the 1st differential as ['3x[super 2]].

To see how this extra information is used to find a root, view
[@http://en.wikipedia.org/wiki/Newton%27s_method Newton-Raphson iterations]
and the [@http://en.wikipedia.org/wiki/Newton%27s_method#mediaviewer/File:NewtonIteration_Ani.gif animation].

We need to define a different functor `cbrt_functor_deriv` that returns
both the evaluation of the function to solve, along with its first derivative:

To \'return\' two values, we use a `std::pair` of floating-point values
(though we could equally have used a std::tuple):
*/

//[root_finding_1_deriv_1

template <class T>
struct cbrt_functor_deriv
{ // Functor also returning 1st derivative.
  cbrt_functor_deriv(T const& to_find_root_of) : a(to_find_root_of)
  { // Constructor stores value a to find root of,
    // for example: calling cbrt_functor_deriv<T>(a) to use to get cube root of a.
  }
  std::pair<T, T> operator()(T const& x)
  { 
    // Return both f(x) and f'(x).
    T fx = x*x*x - a;                // Difference (estimate x^3 - value).
    T dx =  3 * x*x;                 // 1st derivative = 3x^2.
    return std::make_pair(fx, dx);   // 'return' both fx and dx.
  }
private:
  T a;                               // Store value to be 'cube_rooted'.
};

/*`Our cube root function is now:*/

template <class T>
T cbrt_deriv(T x)
{ 
  // return cube root of x using 1st derivative and Newton_Raphson.
  using namespace boost::math::tools;
  int exponent;
  frexp(x, &exponent);                                // Get exponent of z (ignore mantissa).
  T guess = ldexp(1., exponent/3);                    // Rough guess is to divide the exponent by three.
  T min = ldexp(0.5, exponent/3);                     // Minimum possible value is half our guess.
  T max = ldexp(2., exponent/3);                      // Maximum possible value is twice our guess.
  const int digits = std::numeric_limits<T>::digits;  // Maximum possible binary digits accuracy for type T.
  int get_digits = static_cast<int>(digits * 0.6);    // Accuracy doubles with each step, so stop when we have
                                                      // just over half the digits correct.
  const std::uintmax_t maxit = 20;
  std::uintmax_t it = maxit;
  T result = newton_raphson_iterate(cbrt_functor_deriv<T>(x), guess, min, max, get_digits, it);
  return result;
}

//] [/root_finding_1_deriv_1]


/*
[h3:cbrt_2_derivatives Cube root with 1st & 2nd derivative (slope & curvature)]

Finally we define yet another functor `cbrt_functor_2deriv` that returns
both the evaluation of the function to solve,
along with its first *and second* derivatives:

__spaces[''f](x) = 6x

To \'return\' three values, we use a `tuple` of three floating-point values:
*/

//[root_finding_2deriv_1

template <class T>
struct cbrt_functor_2deriv
{ 
  // Functor returning both 1st and 2nd derivatives.
  cbrt_functor_2deriv(T const& to_find_root_of) : a(to_find_root_of)
  { // Constructor stores value a to find root of, for example:
    // calling cbrt_functor_2deriv<T>(x) to get cube root of x,
  }
  std::tuple<T, T, T> operator()(T const& x)
  { 
    // Return both f(x) and f'(x) and f''(x).
    T fx = x*x*x - a;                     // Difference (estimate x^3 - value).
    T dx = 3 * x*x;                       // 1st derivative = 3x^2.
    T d2x = 6 * x;                        // 2nd derivative = 6x.
    return std::make_tuple(fx, dx, d2x);  // 'return' fx, dx and d2x.
  }
private:
  T a; // to be 'cube_rooted'.
};

/*`Our cube root function is now:*/

template <class T>
T cbrt_2deriv(T x)
{ 
  // return cube root of x using 1st and 2nd derivatives and Halley.
  //using namespace std;  // Help ADL of std functions.
  using namespace boost::math::tools;
  int exponent;
  frexp(x, &exponent);                                // Get exponent of z (ignore mantissa).
  T guess = ldexp(1., exponent/3);                    // Rough guess is to divide the exponent by three.
  T min = ldexp(0.5, exponent/3);                     // Minimum possible value is half our guess.
  T max = ldexp(2., exponent/3);                      // Maximum possible value is twice our guess.
  const int digits = std::numeric_limits<T>::digits;  // Maximum possible binary digits accuracy for type T.
  // digits used to control how accurate to try to make the result.
  int get_digits = static_cast<int>(digits * 0.4);    // Accuracy triples with each step, so stop when just
                                                      // over one third of the digits are correct.
  std::uintmax_t maxit = 20;
  T result = halley_iterate(cbrt_functor_2deriv<T>(x), guess, min, max, get_digits, maxit);
  return result;
}

//] [/root_finding_2deriv_1]

//[root_finding_2deriv_lambda

template <class T>
T cbrt_2deriv_lambda(T x)
{
   // return cube root of x using 1st and 2nd derivatives and Halley.
   //using namespace std;  // Help ADL of std functions.
   using namespace boost::math::tools;
   int exponent;
   frexp(x, &exponent);                                // Get exponent of z (ignore mantissa).
   T guess = ldexp(1., exponent / 3);                    // Rough guess is to divide the exponent by three.
   T min = ldexp(0.5, exponent / 3);                     // Minimum possible value is half our guess.
   T max = ldexp(2., exponent / 3);                      // Maximum possible value is twice our guess.
   const int digits = std::numeric_limits<T>::digits;  // Maximum possible binary digits accuracy for type T.
   // digits used to control how accurate to try to make the result.
   int get_digits = static_cast<int>(digits * 0.4);    // Accuracy triples with each step, so stop when just
   // over one third of the digits are correct.
   std::uintmax_t maxit = 20;
   T result = halley_iterate(
      // lambda function:
      [x](const T& g){ return std::make_tuple(g * g * g - x, 3 * g * g, 6 * g); }, 
      guess, min, max, get_digits, maxit);
   return result;
}

//] [/root_finding_2deriv_lambda]
/*

[h3 Fifth-root function]
Let's now suppose we want to find the [*fifth root] of a number ['a].

The equation we want to solve is :

__spaces['f](x) = x[super 5] - a

If your differentiation is a little rusty
(or you are faced with an equation whose complexity is daunting),
then you can get help, for example from the invaluable
[@http://www.wolframalpha.com/ WolframAlpha site.]

For example, entering the command: `differentiate x ^ 5`

or the Wolfram Language command: ` D[x ^ 5, x]`

gives the output: `d/dx(x ^ 5) = 5 x ^ 4`

and to get the second differential, enter: `second differentiate x ^ 5`

or the Wolfram Language command: `D[x ^ 5, { x, 2 }]`

to get the output: `d ^ 2 / dx ^ 2(x ^ 5) = 20 x ^ 3`

To get a reference value, we can enter: [^fifth root 3126]

or: `N[3126 ^ (1 / 5), 50]`

to get a result with a precision of 50 decimal digits:

5.0003199590478625588206333405631053401128722314376

(We could also get a reference value using Boost.Multiprecision - see below).

The 1st and 2nd derivatives of x[super 5] are:

__spaces['f]\'(x) = 5x[super 4]

__spaces['f]\'\'(x) = 20x[super 3]

*/

//[root_finding_fifth_1
//] [/root_finding_fifth_1]


//[root_finding_fifth_functor_2deriv

/*`Using these expressions for the derivatives, the functor is:
*/

template <class T>
struct fifth_functor_2deriv
{ 
  // Functor returning both 1st and 2nd derivatives.
  fifth_functor_2deriv(T const& to_find_root_of) : a(to_find_root_of)
  { /* Constructor stores value a to find root of, for example: */ }

  std::tuple<T, T, T> operator()(T const& x)
  { 
    // Return both f(x) and f'(x) and f''(x).
    T fx = boost::math::pow<5>(x) - a;    // Difference (estimate x^3 - value).
    T dx = 5 * boost::math::pow<4>(x);    // 1st derivative = 5x^4.
    T d2x = 20 * boost::math::pow<3>(x);  // 2nd derivative = 20 x^3
    return std::make_tuple(fx, dx, d2x);  // 'return' fx, dx and d2x.
  }
private:
  T a;                                    // to be 'fifth_rooted'.
}; // struct fifth_functor_2deriv

//] [/root_finding_fifth_functor_2deriv]

//[root_finding_fifth_2deriv

/*`Our fifth-root function is now:
*/

template <class T>
T fifth_2deriv(T x)
{ 
  // return fifth root of x using 1st and 2nd derivatives and Halley.
  using namespace std;                  // Help ADL of std functions.
  using namespace boost::math::tools;   // for halley_iterate.

  int exponent;
  frexp(x, &exponent);                  // Get exponent of z (ignore mantissa).
  T guess = ldexp(1., exponent / 5);    // Rough guess is to divide the exponent by five.
  T min = ldexp(0.5, exponent / 5);     // Minimum possible value is half our guess.
  T max = ldexp(2., exponent / 5);      // Maximum possible value is twice our guess.
  // Stop when slightly more than one of the digits are correct:
  const int digits = static_cast<int>(std::numeric_limits<T>::digits * 0.4); 
  const std::uintmax_t maxit = 50;
  std::uintmax_t it = maxit;
  T result = halley_iterate(fifth_functor_2deriv<T>(x), guess, min, max, digits, it);
  return result;
}

//] [/root_finding_fifth_2deriv]


int example_main()
{
  ex_out << "Root finding  Examples." << std::endl;
  ex_out.precision(std::numeric_limits<double>::max_digits10);
  // Show all possibly significant decimal digits for double.
  // ex_out.precision(std::numeric_limits<double>::digits10);
  // Show all guaranteed significant decimal digits for double.


//[root_finding_main_1
  try
  {
    double threecubed = 27.;   // Value that has an *exactly representable* integer cube root.
    double threecubedp1 = 28.; // Value whose cube root is *not* exactly representable.

    ex_out << "cbrt(28) " << boost::math::cbrt(28.) << std::endl; // boost::math:: version of cbrt.
    ex_out << "std::cbrt(28) " << std::cbrt(28.) << std::endl;    // std:: version of cbrt.
    ex_out <<" cast double " << static_cast<double>(3.0365889718756625194208095785056696355814539772481111) << std::endl;

    // Cube root using bracketing:
    double r = cbrt_noderiv(threecubed);
    ex_out << "cbrt_noderiv(" << threecubed << ") = " << r << std::endl;
    r = cbrt_noderiv(threecubedp1);
    ex_out << "cbrt_noderiv(" << threecubedp1 << ") = " << r << std::endl;
//] [/root_finding_main_1]
    //[root_finding_main_2

    // Cube root using 1st differential Newton-Raphson:
    r = cbrt_deriv(threecubed);
    ex_out << "cbrt_deriv(" << threecubed << ") = " << r << std::endl;
    r = cbrt_deriv(threecubedp1);
    ex_out << "cbrt_deriv(" << threecubedp1 << ") = " << r << std::endl;

    // Cube root using Halley with 1st and 2nd differentials.
    r = cbrt_2deriv(threecubed);
    ex_out << "cbrt_2deriv(" << threecubed << ") = " << r << std::endl;
    r = cbrt_2deriv(threecubedp1);
    ex_out << "cbrt_2deriv(" << threecubedp1 << ") = " << r << std::endl;

    // Cube root using lambda's:
    r = cbrt_2deriv_lambda(threecubed);
    ex_out << "cbrt_2deriv(" << threecubed << ") = " << r << std::endl;
    r = cbrt_2deriv_lambda(threecubedp1);
    ex_out << "cbrt_2deriv(" << threecubedp1 << ") = " << r << std::endl;

    // Fifth root.

    double fivepowfive = 3125; // Example of a value that has an exact integer fifth root.
    // Exact value of fifth root is exactly 5.
    ex_out << "Fifth root  of " << fivepowfive << " is " << 5 << std::endl;

    double fivepowfivep1 = fivepowfive + 1; // Example of a value whose fifth root is *not* exactly representable.
    // Value of fifth root is 5.0003199590478625588206333405631053401128722314376 (50 decimal digits precision)
    // and to std::numeric_limits<double>::max_digits10 double precision (usually 17) is

    double root5v2 = static_cast<double>(5.0003199590478625588206333405631053401128722314376);
        ex_out << "Fifth root  of " << fivepowfivep1 << " is " << root5v2 << std::endl;

    // Using Halley with 1st and 2nd differentials.
    r = fifth_2deriv(fivepowfive);
    ex_out << "fifth_2deriv(" << fivepowfive << ") = " << r << std::endl;
    r = fifth_2deriv(fivepowfivep1);
    ex_out << "fifth_2deriv(" << fivepowfivep1 << ") = " << r << std::endl;
//] [/root_finding_main_?]
  }
  catch(const std::exception& e)
  { // Always useful to include try & catch blocks because default policies
    // are to throw exceptions on arguments that cause errors like underflow, overflow.
    // Lacking try & catch blocks, the program will abort without a message below,
    // which may give some helpful clues as to the cause of the exception.
    ex_out <<
      "\n""Message from thrown exception was:\n   " << e.what() << std::endl;
  }
  return 0;
    return 0;  // main's implicit return
}  // int example_main()

//[root_finding_example_output
/*`
Normal output is:

[pre
  root_finding_example.cpp
  Generating code
  Finished generating code
  root_finding_example.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Release\root_finding_example.exe
  Cube Root finding (cbrt) Example.
  Iterations 10
  cbrt_1(27) = 3
  Iterations 10
  Unable to locate solution in chosen iterations: Current best guess is between 3.0365889718756613 and 3.0365889718756627
  cbrt_1(28) = 3.0365889718756618
  cbrt_1(27) = 3
  cbrt_2(28) = 3.0365889718756627
  Iterations 4
  cbrt_3(27) = 3
  Iterations 5
  cbrt_3(28) = 3.0365889718756627

] [/pre]

to get some (much!) diagnostic output we can add

#define BOOST_MATH_INSTRUMENT

[pre

]
*/
//] [/root_finding_example_output]

/*

cbrt(28) 3.0365889718756622
std::cbrt(28) 3.0365889718756627

*/

TEST_CASE("example: root_finding_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Root finding  Examples.
cbrt(28) 3.0365889718756627
std::cbrt(28) 3.0365889718756631
 cast double 3.0365889718756627
cbrt_noderiv(27) = 3
cbrt_noderiv(28) = 3.0365889718756618
cbrt_deriv(27) = 3
cbrt_deriv(28) = 3.0365889718756627
cbrt_2deriv(27) = 3
cbrt_2deriv(28) = 3.0365889718756627
cbrt_2deriv(27) = 3
cbrt_2deriv(28) = 3.0365889718756627
Fifth root  of 3125 is 5
Fifth root  of 3126 is 5.0003199590478626
fifth_2deriv(3125) = 5
fifth_2deriv(3126) = 5.0003199590478626
)ex"));
#endif
}
}  // namespace ex_root_finding_example

// libs/math/example/skew_normal_example.cpp
namespace ex_skew_normal_example {
#ifdef _MSC_VER
#  pragma warning (disable : 4512) // assignment operator could not be generated
#  pragma warning (disable : 4127) // conditional expression is constant.
#endif

using boost::math::skew_normal_distribution;
using boost::math::skew_normal;

void check(const double loc, const double sc, const double sh,
     const double * const cumulants, const std::pair<double, double> qu,
     const double x, const double tpdf, const double tcdf)
{
  using namespace boost::math;

  skew_normal D(loc, sc, sh);
  
  const double sk = cumulants[2] / (std::pow(cumulants[1], 1.5));
  const double kt = cumulants[3] / (cumulants[1] * cumulants[1]);

  // checks against tabulated values
  ex_out << "mean: table=" << cumulants[0] << "\tcompute=" << mean(D) << "\tdiff=" << fabs(cumulants[0]-mean(D)) << std::endl;
  ex_out << "var: table=" << cumulants[1] << "\tcompute=" << variance(D) << "\tdiff=" << fabs(cumulants[1]-variance(D)) << std::endl;
  ex_out << "skew: table=" << sk << "\tcompute=" << skewness(D) << "\tdiff=" << fabs(sk-skewness(D)) << std::endl;
  ex_out << "kur.: table=" << kt << "\tcompute=" << kurtosis_excess(D) << "\tdiff=" << fabs(kt-kurtosis_excess(D)) << std::endl;
  ex_out << "mode: table=" << "N/A" << "\tcompute=" << mode(D) << "\tdiff=" << "N/A" << std::endl;

  const double q = quantile(D, qu.first);
  const double cq = quantile(complement(D, qu.first));

  ex_out << "quantile(" << qu.first << "): table=" << qu.second << "\tcompute=" << q << "\tdiff=" << fabs(qu.second-q) << std::endl;

  // consistency
  ex_out << "cdf(quantile)=" << cdf(D, q) << "\tp=" << qu.first << "\tdiff=" << fabs(qu.first-cdf(D, q)) << std::endl;
  ex_out << "ccdf(cquantile)=" << cdf(complement(D,cq)) << "\tp=" << qu.first << "\tdiff=" << fabs(qu.first-cdf(complement(D,cq))) << std::endl;

  // PDF & CDF
  ex_out << "pdf(" << x << "): table=" << tpdf << "\tcompute=" << pdf(D,x) << "\tdiff=" << fabs(tpdf-pdf(D,x)) << std::endl;
  ex_out << "cdf(" << x << "): table=" << tcdf << "\tcompute=" << cdf(D,x) << "\tdiff=" << fabs(tcdf-cdf(D,x)) << std::endl;
  ex_out << "================================\n";
}

int example_main()
{
  using namespace boost::math;

  double sc = 0.0,loc,sh,x,dsn,qsn,psn,p;
  ex_out << std::setprecision(20);

  double cumulants[4];


  /* R:
     > install.packages("sn")
     Warning in install.packages("sn") :
     'lib = "/usr/lib64/R/library"' is not writable
     Would you like to create a personal library
     '~/R/x86_64-unknown-linux-gnu-library/2.12'
     to install packages into?  (y/n) y
     --- Please select a CRAN mirror for use in this session ---
     Loading Tcl/Tk interface ... done
     also installing the dependency mnormt

     trying URL 'http://mirrors.dotsrc.org/cran/src/contrib/mnormt_1.4-5.tar.gz'
     Content type 'application/x-gzip' length 34049 bytes (33 Kb)
     opened URL
     ==================================================
     downloaded 33 Kb

     trying URL 'http://mirrors.dotsrc.org/cran/src/contrib/sn_0.4-17.tar.gz'
     Content type 'application/x-gzip' length 65451 bytes (63 Kb)
     opened URL
     ==================================================
     downloaded 63 Kb


     > library(sn)
     > options(digits=22)


     > sn.cumulants(1.1, 2.2, -3.3)
     [1] -0.5799089925398568  2.0179057767837230 -2.0347951542374196
     [4]  2.2553488991015072
     > qsn(0.3, 1.1, 2.2, -3.3)
     [1] -1.180104068086876
     > psn(0.4, 1.1, 2.2, -3.3)
     [1] 0.733918618927874
     > dsn(0.4, 1.1, 2.2, -3.3)
     [1] 0.2941401101565995

   */

  //1 st
  loc = 1.1; sc = 2.2; sh = -3.3;
  ex_out << "location: " << loc << "\tscale: " << sc << "\tshape: " << sh << std::endl;
  cumulants[0] = -0.5799089925398568;
  cumulants[1] =  2.0179057767837230;
  cumulants[2] = -2.0347951542374196;
  cumulants[3] =  2.2553488991015072;
  x = 0.4;
  p = 0.3;
  qsn = -1.180104068086876;
  psn = 0.733918618927874;
  dsn = 0.2941401101565995;

  check(loc, sc, sh, cumulants, std::make_pair(p,qsn), x, dsn, psn);

  /* R:

     > sn.cumulants(1.1, .02, .03)
     [1] 1.1004785154529559e+00 3.9977102296128255e-04 4.7027439329779991e-11
     [4] 1.4847542790693825e-14
     > qsn(0.01, 1.1, .02, .03)
     [1] 1.053964962950150
     > psn(1.3, 1.1, .02, .03)
     [1] 1
     > dsn(1.3, 1.1, .02, .03)
     [1] 4.754580380601393e-21

   */

  // 2nd
  loc = 1.1; sc = .02; sh = .03;
  ex_out << "location: " << loc << "\tscale: " << sc << "\tshape: " << sh << std::endl;
  cumulants[0] = 1.1004785154529559e+00;
  cumulants[1] = 3.9977102296128255e-04;
  cumulants[2] = 4.7027439329779991e-11;
  cumulants[3] = 1.4847542790693825e-14;
  x = 1.3;
  p = 0.01;
  qsn = 1.053964962950150;
  psn = 1;
  dsn = 4.754580380601393e-21;

  check(loc, sc, sh, cumulants, std::make_pair(p,qsn), x, dsn, psn);

  /* R:

     > sn.cumulants(10.1, 5, -.03)
     [1]  9.980371136761052e+00  2.498568893508016e+01 -7.348037395278123e-04
     [4]  5.799821402614775e-05
     > qsn(.8, 10.1, 5, -.03)
     [1] 14.18727407491953
     > psn(-1.3, 10.1, 5, -.03)
     [1] 0.01201290665838824
     > dsn(-1.3, 10.1, 5, -.03)
     [1] 0.006254346348897927


  */

  // 3rd
  loc = 10.1; sc = 5; sh = -.03;
  ex_out << "location: " << loc << "\tscale: " << sc << "\tshape: " << sh << std::endl;
  cumulants[0] = 9.980371136761052e+00;
  cumulants[1] = 2.498568893508016e+01;
  cumulants[2] = -7.348037395278123e-04;
  cumulants[3] = 5.799821402614775e-05;
  x = -1.3;
  p = 0.8;
  qsn = 14.18727407491953;
  psn = 0.01201290665838824;
  dsn = 0.006254346348897927;

  check(loc, sc, sh, cumulants, std::make_pair(p,qsn), x, dsn, psn);


  /* R:

     > sn.cumulants(-10.1, 5, 30)
     [1] -6.112791696741384  9.102169946425548 27.206345266148194 71.572537838642916
     > qsn(.8, -10.1, 5, 30)
     [1] -3.692242172277
     > psn(-1.3, -10.1, 5, 30)
     [1] 0.921592193425035
     > dsn(-1.3, -10.1, 5, 30)
     [1] 0.0339105445232089

  */

  // 4th
  loc = -10.1; sc = 5; sh = 30;
  ex_out << "location: " << loc << "\tscale: " << sc << "\tshape: " << sh << std::endl;
  cumulants[0] = -6.112791696741384;
  cumulants[1] = 9.102169946425548;
  cumulants[2] = 27.206345266148194;
  cumulants[3] = 71.572537838642916;
  x = -1.3;
  p = 0.8;
  qsn = -3.692242172277;
  psn = 0.921592193425035;
  dsn = 0.0339105445232089;

  check(loc, sc, sh, cumulants, std::make_pair(p,qsn), x, dsn, psn);
  
  
  /* R:

     > sn.cumulants(0,1,5)
     [1] 0.7823901817554269 0.3878656034927102 0.2055576317962637 0.1061119471655128
     > qsn(0.5,0,1,5)
     [1] 0.674471117502844
     > psn(-0.5, 0,1,5)
     [1] 0.0002731513884140924
     > dsn(-0.5, 0,1,5)
     [1] 0.00437241570403263

  */

  // 5th
  loc = 0; sc = 1; sh = 5;
  ex_out << "location: " << loc << "\tscale: " << sc << "\tshape: " << sh << std::endl;
  cumulants[0] = 0.7823901817554269;
  cumulants[1] = 0.3878656034927102;
  cumulants[2] = 0.2055576317962637;
  cumulants[3] = 0.1061119471655128;
  x = -0.5;
  p = 0.5;
  qsn = 0.674471117502844;
  psn = 0.0002731513884140924;
  dsn = 0.00437241570403263;

  check(loc, sc, sh, cumulants, std::make_pair(p,qsn), x, dsn, psn);

  /* R:

     > sn.cumulants(0,1,1e5)
     [1] 0.7978845607629713 0.3633802276960805 0.2180136141122883 0.1147706820312645
     > qsn(0.5,0,1,1e5)
     [1] 0.6744897501960818
     > psn(-0.5, 0,1,1e5)
     [1] 0
     > dsn(-0.5, 0,1,1e5)
     [1] 0

  */

  // 6th
  loc = 0; sc = 1; sh = 1e5;
  ex_out << "location: " << loc << "\tscale: " << sc << "\tshape: " << sh << std::endl;
  cumulants[0] = 0.7978845607629713;
  cumulants[1] = 0.3633802276960805;
  cumulants[2] = 0.2180136141122883;
  cumulants[3] = 0.1147706820312645;
  x = -0.5;
  p = 0.5;
  qsn = 0.6744897501960818;
  psn = 0.;
  dsn = 0.;

  check(loc, sc, sh, cumulants, std::make_pair(p,qsn), x, dsn, psn);

  return 0;
    return 0;  // main's implicit return
}


// EOF

TEST_CASE("example: skew_normal_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(location: 1.1000000000000000888	scale: 2.2000000000000001776	shape: -3.2999999999999998224
mean: table=-0.57990899253985683792	compute=-0.57990899253985683792	diff=0
var: table=2.0179057767837229598	compute=2.0179057767837234039	diff=4.4408920985006261617e-16
skew: table=-0.70985454817153847973	compute=-0.70985454817153792462	diff=5.5511151231257827021e-16
kur.: table=0.55387526252417818018	compute=0.55387526252417906836	diff=8.8817841970012523234e-16
mode: table=N/A	compute=0.097807239297288184332	diff=N/A
quantile(0.2999999999999999889): table=-1.1801040680868759569	compute=-1.1801040680868757349	diff=2.2204460492503130808e-16
cdf(quantile)=0.29999999999999993339	p=0.2999999999999999889	diff=5.5511151231257827021e-17
ccdf(cquantile)=0.29999999999999993339	p=0.2999999999999999889	diff=5.5511151231257827021e-17
pdf(0.4000000000000000222): table=0.29414011015659952131	compute=0.29414011015659952131	diff=0
cdf(0.4000000000000000222): table=0.73391861892787402244	compute=0.73391861892787380039	diff=2.2204460492503130808e-16
================================
location: 1.1000000000000000888	scale: 0.020000000000000000416	shape: 0.02999999999999999889
mean: table=1.100478515452955941	compute=1.100478515452955941	diff=0
var: table=0.000399771022961282552	compute=0.000399771022961282552	diff=0
skew: table=5.8834811259890419489e-06	compute=5.8834811259890385608e-06	diff=3.3881317890172013563e-21
kur.: table=9.2903475812137595958e-08	compute=9.2903475812137807716e-08	diff=2.1175823681357508477e-22
mode: table=N/A	compute=1.1004784566347154406	diff=N/A
quantile(0.010000000000000000208): table=1.0539649629501499639	compute=1.053964962950150408	diff=4.4408920985006261617e-16
cdf(quantile)=0.010000000000000045311	p=0.010000000000000000208	diff=4.5102810375396984455e-17
ccdf(cquantile)=0.010000000000000137251	p=0.010000000000000000208	diff=1.3704315460216776046e-16
pdf(1.3000000000000000444): table=4.7545803806013929087e-21	compute=4.7545803806013929087e-21	diff=0
cdf(1.3000000000000000444): table=1	compute=1	diff=0
================================
location: 10.099999999999999645	scale: 5	shape: -0.02999999999999999889
mean: table=9.9803711367610521421	compute=9.9803711367610521421	diff=0
var: table=24.985688935080158757	compute=24.985688935080158757	diff=0
skew: table=-5.8834811259890411019e-06	compute=-5.8834811259890385608e-06	diff=2.5410988417629010172e-21
kur.: table=9.2903475812137595958e-08	compute=9.2903475812137807716e-08	diff=2.1175823681357508477e-22
mode: table=N/A	compute=9.9803858413211834488	diff=N/A
quantile(0.80000000000000004441): table=14.18727407491953052	compute=14.187274074919534073	diff=3.5527136788005009294e-15
cdf(quantile)=0.80000000000000015543	p=0.80000000000000004441	diff=1.1102230246251565404e-16
ccdf(cquantile)=0.80000000000000004441	p=0.80000000000000004441	diff=0
pdf(-1.3000000000000000444): table=0.0062543463488979270798	compute=0.0062543463488979270798	diff=0
cdf(-1.3000000000000000444): table=0.012012906658388240039	compute=0.012012906658388243508	diff=3.4694469519536141888e-18
================================
location: -10.099999999999999645	scale: 5	shape: 30
mean: table=-6.1127916967413842286	compute=-6.1127916967413833405	diff=8.8817841970012523234e-16
var: table=9.1021699464255476641	compute=9.1021699464255476641	diff=0
skew: table=0.9907242544368699555	compute=0.99072425443686928936	diff=6.6613381477509392425e-16
kur.: table=0.86388620084060674387	compute=0.86388620084060874227	diff=1.9984014443252817728e-15
mode: table=N/A	compute=-9.5858670159316403669	diff=N/A
quantile(0.80000000000000004441): table=-3.692242172277000023	compute=-3.6922421722769982466	diff=1.7763568394002504647e-15
cdf(quantile)=0.80000000000000004441	p=0.80000000000000004441	diff=0
ccdf(cquantile)=0.80000000000000004441	p=0.80000000000000004441	diff=0
pdf(-1.3000000000000000444): table=0.033910544523208900614	compute=0.033910544523208900614	diff=0
cdf(-1.3000000000000000444): table=0.92159219342503495565	compute=0.92159219342503462258	diff=3.3306690738754696213e-16
================================
location: 0	scale: 1	shape: 5
mean: table=0.78239018175542685007	compute=0.78239018175542685007	diff=0
var: table=0.3878656034927102092	compute=0.38786560349271026471	diff=5.5511151231257827021e-17
skew: table=0.85096501263137191984	compute=0.8509650126313712537	diff=6.6613381477509392425e-16
kur.: table=0.70534525503313716399	compute=0.70534525503313816319	diff=9.9920072216264088638e-16
mode: table=N/A	compute=0.37050182855364710566	diff=N/A
quantile(0.5): table=0.67447111750284405307	compute=0.67447111750284394205	diff=1.1102230246251565404e-16
cdf(quantile)=0.5	p=0.5	diff=0
ccdf(cquantile)=0.5	p=0.5	diff=0
pdf(-0.5): table=0.0043724157040326300933	compute=0.0043724157040326335627	diff=3.4694469519536141888e-18
cdf(-0.5): table=0.00027315138841409240767	compute=0.00027315138841410568915	diff=1.3281476612947429317e-17
================================
location: 0	scale: 1	shape: 100000
mean: table=0.79788456076297131769	compute=0.79788456076297120667	diff=1.1102230246251565404e-16
var: table=0.36338022769608052664	compute=0.36338022769608058216	diff=5.5511151231257827021e-17
skew: table=0.99527174602031842632	compute=0.99527174602031909245	diff=6.6613381477509392425e-16
kur.: table=0.86917730312758856304	compute=0.86917730312758978428	diff=1.2212453270876721945e-15
mode: table=N/A	compute=6.3649211392827694553e-05	diff=N/A
quantile(0.5): table=0.67448975019608181647	compute=0.67448975019608181647	diff=0
cdf(quantile)=0.5	p=0.5	diff=0
ccdf(cquantile)=0.5	p=0.5	diff=0
pdf(-0.5): table=0	compute=0	diff=0
cdf(-0.5): table=0	compute=0	diff=0
================================
)ex"));
}
}  // namespace ex_skew_normal_example

// libs/math/example/students_t_single_sample.cpp
namespace ex_students_t_single_sample {
#ifdef _MSC_VER
#  pragma warning(disable: 4512) // assignment operator could not be generated.
#  pragma warning(disable: 4510) // default constructor could not be generated.
#  pragma warning(disable: 4610) // can never be instantiated - user defined constructor required.
#endif


// avoid "using namespace std;" and "using namespace boost::math;"
// to avoid potential ambiguity with names in std random.
 using std::endl;
using std::left; using std::fixed; using std::right; using std::scientific;
using std::setw;
using std::setprecision;

void confidence_limits_on_mean(double Sm, double Sd, unsigned Sn)
{
   //
   // Sm = Sample Mean.
   // Sd = Sample Standard Deviation.
   // Sn = Sample Size.
   //
   // Calculate confidence intervals for the mean.
   // For example if we set the confidence limit to
   // 0.95, we know that if we repeat the sampling
   // 100 times, then we expect that the true mean
   // will be between out limits on 95 occasions.
   // Note: this is not the same as saying a 95%
   // confidence interval means that there is a 95%
   // probability that the interval contains the true mean.
   // The interval computed from a given sample either
   // contains the true mean or it does not.
   // See http://www.itl.nist.gov/div898/handbook/eda/section3/eda352.htm

   using boost::math::students_t;

   // Print out general info:
   ex_out <<
      "__________________________________\n"
      "2-Sided Confidence Limits For Mean\n"
      "__________________________________\n\n";
   ex_out << setprecision(7);
   ex_out << setw(40) << left << "Number of Observations" << "=  " << Sn << "\n";
   ex_out << setw(40) << left << "Mean" << "=  " << Sm << "\n";
   ex_out << setw(40) << left << "Standard Deviation" << "=  " << Sd << "\n";
   //
   // Define a table of significance/risk levels:
   //
   double alpha[] = { 0.5, 0.25, 0.1, 0.05, 0.01, 0.001, 0.0001, 0.00001 };
   //
   // Start by declaring the distribution we'll need:
   //
   students_t dist(Sn - 1);
   //
   // Print table header:
   //
   ex_out << "\n\n"
           "_______________________________________________________________\n"
           "Confidence       T           Interval          Lower          Upper\n"
           " Value (%)     Value          Width            Limit          Limit\n"
           "_______________________________________________________________\n";
   //
   // Now print out the data for the table rows.
   //
   for(unsigned i = 0; i < sizeof(alpha)/sizeof(alpha[0]); ++i)
   {
      // Confidence value:
      ex_out << fixed << setprecision(3) << setw(10) << right << 100 * (1-alpha[i]);
      // calculate T:
      double T = quantile(complement(dist, alpha[i] / 2));
      // Print T:
      ex_out << fixed << setprecision(3) << setw(10) << right << T;
      // Calculate width of interval (one sided):
      double w = T * Sd / sqrt(double(Sn));
      // Print width:
      if(w < 0.01)
         ex_out << scientific << setprecision(3) << setw(17) << right << w;
      else
         ex_out << fixed << setprecision(3) << setw(17) << right << w;
      // Print Limits:
      ex_out << fixed << setprecision(5) << setw(15) << right << Sm - w;
      ex_out << fixed << setprecision(5) << setw(15) << right << Sm + w << endl;
   }
   ex_out << endl;
} // void confidence_limits_on_mean

void single_sample_t_test(double M, double Sm, double Sd, unsigned Sn, double alpha)
{
   //
   // M = true mean.
   // Sm = Sample Mean.
   // Sd = Sample Standard Deviation.
   // Sn = Sample Size.
   // alpha = Significance Level.
   //
   // A Students t test applied to a single set of data.
   // We are testing the null hypothesis that the true
   // mean of the sample is M, and that any variation is down
   // to chance.  We can also test the alternative hypothesis
   // that any difference is not down to chance.
   // See http://www.itl.nist.gov/div898/handbook/eda/section3/eda352.htm
   
   using boost::math::students_t;

   // Print header:
   ex_out <<
      "__________________________________\n"
      "Student t test for a single sample\n"
      "__________________________________\n\n";
   ex_out << setprecision(5);
   ex_out << setw(55) << left << "Number of Observations" << "=  " << Sn << "\n";
   ex_out << setw(55) << left << "Sample Mean" << "=  " << Sm << "\n";
   ex_out << setw(55) << left << "Sample Standard Deviation" << "=  " << Sd << "\n";
   ex_out << setw(55) << left << "Expected True Mean" << "=  " << M << "\n\n";
   //
   // Now we can calculate and output some stats:
   //
   // Difference in means:
   double diff = Sm - M;
   ex_out << setw(55) << left << "Sample Mean - Expected Test Mean" << "=  " << diff << "\n";
   // Degrees of freedom:
   unsigned v = Sn - 1;
   ex_out << setw(55) << left << "Degrees of Freedom" << "=  " << v << "\n";
   // t-statistic:
   double t_stat = diff * sqrt(double(Sn)) / Sd;
   ex_out << setw(55) << left << "T Statistic" << "=  " << t_stat << "\n";
   //
   // Finally define our distribution, and get the probability:
   //
   students_t dist(v);
   double q = cdf(complement(dist, fabs(t_stat)));
   ex_out << setw(55) << left << "Probability that difference is due to chance" << "=  "
      << setprecision(3) << scientific << 2 * q << "\n\n";
   //
   // Finally print out results of alternative hypothesis:
   //
   ex_out << setw(55) << left <<
      "Results for Alternative Hypothesis and alpha" << "=  "
      << setprecision(4) << fixed << alpha << "\n\n";
   ex_out << "Alternative Hypothesis     Conclusion\n";
   ex_out << "Mean != " << setprecision(3) << fixed << M << "            ";
   if(q < alpha / 2)
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << "Mean  < " << setprecision(3) << fixed << M << "            ";
   if(cdf(complement(dist, t_stat)) > alpha)
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << "Mean  > " << setprecision(3) << fixed << M << "            ";
   if(cdf(dist, t_stat) > alpha)
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << endl << endl;
} // void single_sample_t_test(

void single_sample_find_df(double M, double Sm, double Sd)
{
   //
   // M = true mean.
   // Sm = Sample Mean.
   // Sd = Sample Standard Deviation.
   //
 
   using boost::math::students_t;

   // Print out general info:
   ex_out <<
      "_____________________________________________________________\n"
      "Estimated sample sizes required for various confidence levels\n"
      "_____________________________________________________________\n\n";
   ex_out << setprecision(5);
   ex_out << setw(40) << left << "True Mean" << "=  " << M << "\n";
   ex_out << setw(40) << left << "Sample Mean" << "=  " << Sm << "\n";
   ex_out << setw(40) << left << "Sample Standard Deviation" << "=  " << Sd << "\n";
   //
   // Define a table of significance intervals:
   //
   double alpha[] = { 0.5, 0.25, 0.1, 0.05, 0.01, 0.001, 0.0001, 0.00001 };
   //
   // Print table header:
   //
   ex_out << "\n\n"
           "_______________________________________________________________\n"
           "Confidence       Estimated          Estimated\n"
           " Value (%)      Sample Size        Sample Size\n"
           "              (one sided test)    (two sided test)\n"
           "_______________________________________________________________\n";
   //
   // Now print out the data for the table rows.
   //
   for(unsigned i = 1; i < sizeof(alpha)/sizeof(alpha[0]); ++i)
   {
      // Confidence value:
      ex_out << fixed << setprecision(3) << setw(10) << right << 100 * (1-alpha[i]);
      // calculate df for single sided test:
      double df = students_t::find_degrees_of_freedom(
         fabs(M - Sm), alpha[i], alpha[i], Sd);
      // convert to sample size, always one more than the degrees of freedom:
      double size = ceil(df) + 1;
      // Print size:
      ex_out << fixed << setprecision(0) << setw(16) << right << size;
      // calculate df for two sided test:
      df = students_t::find_degrees_of_freedom(
         fabs(M - Sm), alpha[i]/2, alpha[i], Sd);
      // convert to sample size:
      size = ceil(df) + 1;
      // Print size:
      ex_out << fixed << setprecision(0) << setw(16) << right << size << endl;
   }
   ex_out << endl;
} // void single_sample_find_df

int example_main()
{
   //
   // Run tests for Heat Flow Meter data
   // see http://www.itl.nist.gov/div898/handbook/eda/section4/eda428.htm
   // The data was collected while calibrating a heat flow meter
   // against a known value.
   //
   confidence_limits_on_mean(9.261460, 0.2278881e-01, 195);
   single_sample_t_test(5, 9.261460, 0.2278881e-01, 195, 0.05);
   single_sample_find_df(5, 9.261460, 0.2278881e-01);

   //
   // Data for this example from:
   // P.K.Hou, O. W. Lau & M.C. Wong, Analyst (1983) vol. 108, p 64.
   // from Statistics for Analytical Chemistry, 3rd ed. (1994), pp 54-55
   // J. C. Miller and J. N. Miller, Ellis Horwood ISBN 0 13 0309907
   //
   // Determination of mercury by cold-vapour atomic absorption,
   // the following values were obtained fusing a trusted
   // Standard Reference Material containing 38.9% mercury,
   // which we assume is correct or 'true'.
   //
   confidence_limits_on_mean(37.8, 0.964365, 3);
   // 95% test:
   single_sample_t_test(38.9, 37.8, 0.964365, 3, 0.05);
   // 90% test:
   single_sample_t_test(38.9, 37.8, 0.964365, 3, 0.1);
   // parameter estimate:
   single_sample_find_df(38.9, 37.8, 0.964365);

   return 0;
    return 0;  // main's implicit return
} // int example_main()

/*

Output:

------ Rebuild All started: Project: students_t_single_sample, Configuration: Release Win32 ------
  students_t_single_sample.cpp
  Generating code
  Finished generating code
  students_t_single_sample.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Release\students_t_single_sample.exe
__________________________________
2-Sided Confidence Limits For Mean
__________________________________

Number of Observations                  =  195
Mean                                    =  9.26146
Standard Deviation                      =  0.02278881


_______________________________________________________________
Confidence       T           Interval          Lower          Upper
 Value (%)     Value          Width            Limit          Limit
_______________________________________________________________
    50.000     0.676       1.103e-003        9.26036        9.26256
    75.000     1.154       1.883e-003        9.25958        9.26334
    90.000     1.653       2.697e-003        9.25876        9.26416
    95.000     1.972       3.219e-003        9.25824        9.26468
    99.000     2.601       4.245e-003        9.25721        9.26571
    99.900     3.341       5.453e-003        9.25601        9.26691
    99.990     3.973       6.484e-003        9.25498        9.26794
    99.999     4.537       7.404e-003        9.25406        9.26886

__________________________________
Student t test for a single sample
__________________________________

Number of Observations                                 =  195
Sample Mean                                            =  9.26146
Sample Standard Deviation                              =  0.02279
Expected True Mean                                     =  5.00000

Sample Mean - Expected Test Mean                       =  4.26146
Degrees of Freedom                                     =  194
T Statistic                                            =  2611.28380
Probability that difference is due to chance           =  0.000e+000

Results for Alternative Hypothesis and alpha           =  0.0500

Alternative Hypothesis     Conclusion
Mean != 5.000            NOT REJECTED
Mean  < 5.000            REJECTED
Mean  > 5.000            NOT REJECTED


_____________________________________________________________
Estimated sample sizes required for various confidence levels
_____________________________________________________________

True Mean                               =  5.00000
Sample Mean                             =  9.26146
Sample Standard Deviation               =  0.02279


_______________________________________________________________
Confidence       Estimated          Estimated
 Value (%)      Sample Size        Sample Size
              (one sided test)    (two sided test)
_______________________________________________________________
    75.000               2               2
    90.000               2               2
    95.000               2               2
    99.000               2               2
    99.900               3               3
    99.990               3               3
    99.999               4               4

__________________________________
2-Sided Confidence Limits For Mean
__________________________________

Number of Observations                  =  3
Mean                                    =  37.8000000
Standard Deviation                      =  0.9643650


_______________________________________________________________
Confidence       T           Interval          Lower          Upper
 Value (%)     Value          Width            Limit          Limit
_______________________________________________________________
    50.000     0.816            0.455       37.34539       38.25461
    75.000     1.604            0.893       36.90717       38.69283
    90.000     2.920            1.626       36.17422       39.42578
    95.000     4.303            2.396       35.40438       40.19562
    99.000     9.925            5.526       32.27408       43.32592
    99.900    31.599           17.594       20.20639       55.39361
    99.990    99.992           55.673      -17.87346       93.47346
    99.999   316.225          176.067     -138.26683      213.86683

__________________________________
Student t test for a single sample
__________________________________

Number of Observations                                 =  3
Sample Mean                                            =  37.80000
Sample Standard Deviation                              =  0.96437
Expected True Mean                                     =  38.90000

Sample Mean - Expected Test Mean                       =  -1.10000
Degrees of Freedom                                     =  2
T Statistic                                            =  -1.97566
Probability that difference is due to chance           =  1.869e-001

Results for Alternative Hypothesis and alpha           =  0.0500

Alternative Hypothesis     Conclusion
Mean != 38.900            REJECTED
Mean  < 38.900            NOT REJECTED
Mean  > 38.900            NOT REJECTED


__________________________________
Student t test for a single sample
__________________________________

Number of Observations                                 =  3
Sample Mean                                            =  37.80000
Sample Standard Deviation                              =  0.96437
Expected True Mean                                     =  38.90000

Sample Mean - Expected Test Mean                       =  -1.10000
Degrees of Freedom                                     =  2
T Statistic                                            =  -1.97566
Probability that difference is due to chance           =  1.869e-001

Results for Alternative Hypothesis and alpha           =  0.1000

Alternative Hypothesis     Conclusion
Mean != 38.900            REJECTED
Mean  < 38.900            NOT REJECTED
Mean  > 38.900            REJECTED


_____________________________________________________________
Estimated sample sizes required for various confidence levels
_____________________________________________________________

True Mean                               =  38.90000
Sample Mean                             =  37.80000
Sample Standard Deviation               =  0.96437


_______________________________________________________________
Confidence       Estimated          Estimated
 Value (%)      Sample Size        Sample Size
              (one sided test)    (two sided test)
_______________________________________________________________
    75.000               3               4
    90.000               7               9
    95.000              11              13
    99.000              20              22
    99.900              35              37
    99.990              50              53
    99.999              66              68

*/

TEST_CASE("example: students_t_single_sample") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(__________________________________
2-Sided Confidence Limits For Mean
__________________________________

Number of Observations                  =  195
Mean                                    =  9.26146
Standard Deviation                      =  0.02278881


_______________________________________________________________
Confidence       T           Interval          Lower          Upper
 Value (%)     Value          Width            Limit          Limit
_______________________________________________________________
    50.000     0.676        1.103e-03        9.26036        9.26256
    75.000     1.154        1.883e-03        9.25958        9.26334
    90.000     1.653        2.697e-03        9.25876        9.26416
    95.000     1.972        3.219e-03        9.25824        9.26468
    99.000     2.601        4.245e-03        9.25721        9.26571
    99.900     3.341        5.453e-03        9.25601        9.26691
    99.990     3.973        6.484e-03        9.25498        9.26794
    99.999     4.537        7.404e-03        9.25406        9.26886

__________________________________
Student t test for a single sample
__________________________________

Number of Observations                                 =  195
Sample Mean                                            =  9.26146
Sample Standard Deviation                              =  0.02279
Expected True Mean                                     =  5.00000

Sample Mean - Expected Test Mean                       =  4.26146
Degrees of Freedom                                     =  194
T Statistic                                            =  2611.28380
Probability that difference is due to chance           =  0.000e+00

Results for Alternative Hypothesis and alpha           =  0.0500

Alternative Hypothesis     Conclusion
Mean != 5.000            NOT REJECTED
Mean  < 5.000            REJECTED
Mean  > 5.000            NOT REJECTED


_____________________________________________________________
Estimated sample sizes required for various confidence levels
_____________________________________________________________

True Mean                               =  5.00000
Sample Mean                             =  9.26146
Sample Standard Deviation               =  0.02279


_______________________________________________________________
Confidence       Estimated          Estimated
 Value (%)      Sample Size        Sample Size
              (one sided test)    (two sided test)
_______________________________________________________________
    75.000               2               2
    90.000               2               2
    95.000               2               2
    99.000               2               2
    99.900               3               3
    99.990               3               3
    99.999               4               4

__________________________________
2-Sided Confidence Limits For Mean
__________________________________

Number of Observations                  =  3
Mean                                    =  37.8000000
Standard Deviation                      =  0.9643650


_______________________________________________________________
Confidence       T           Interval          Lower          Upper
 Value (%)     Value          Width            Limit          Limit
_______________________________________________________________
    50.000     0.816            0.455       37.34539       38.25461
    75.000     1.604            0.893       36.90717       38.69283
    90.000     2.920            1.626       36.17422       39.42578
    95.000     4.303            2.396       35.40438       40.19562
    99.000     9.925            5.526       32.27408       43.32592
    99.900    31.599           17.594       20.20639       55.39361
    99.990    99.992           55.673      -17.87346       93.47346
    99.999   316.225          176.067     -138.26683      213.86683

__________________________________
Student t test for a single sample
__________________________________

Number of Observations                                 =  3
Sample Mean                                            =  37.80000
Sample Standard Deviation                              =  0.96437
Expected True Mean                                     =  38.90000

Sample Mean - Expected Test Mean                       =  -1.10000
Degrees of Freedom                                     =  2
T Statistic                                            =  -1.97566
Probability that difference is due to chance           =  1.869e-01

Results for Alternative Hypothesis and alpha           =  0.0500

Alternative Hypothesis     Conclusion
Mean != 38.900            REJECTED
Mean  < 38.900            NOT REJECTED
Mean  > 38.900            NOT REJECTED


__________________________________
Student t test for a single sample
__________________________________

Number of Observations                                 =  3
Sample Mean                                            =  37.80000
Sample Standard Deviation                              =  0.96437
Expected True Mean                                     =  38.90000

Sample Mean - Expected Test Mean                       =  -1.10000
Degrees of Freedom                                     =  2
T Statistic                                            =  -1.97566
Probability that difference is due to chance           =  1.869e-01

Results for Alternative Hypothesis and alpha           =  0.1000

Alternative Hypothesis     Conclusion
Mean != 38.900            REJECTED
Mean  < 38.900            NOT REJECTED
Mean  > 38.900            REJECTED


_____________________________________________________________
Estimated sample sizes required for various confidence levels
_____________________________________________________________

True Mean                               =  38.90000
Sample Mean                             =  37.80000
Sample Standard Deviation               =  0.96437


_______________________________________________________________
Confidence       Estimated          Estimated
 Value (%)      Sample Size        Sample Size
              (one sided test)    (two sided test)
_______________________________________________________________
    75.000               3               4
    90.000               7               9
    95.000              11              13
    99.000              20              22
    99.900              35              37
    99.990              50              53
    99.999              66              68

)ex"));
}
}  // namespace ex_students_t_single_sample

// libs/math/example/trapezoidal_example.cpp
namespace ex_trapezoidal_example {
int example_main()
{
    using boost::math::constants::two_pi;
    using boost::math::constants::third;
    using boost::math::quadrature::trapezoidal;
    // This function has an analytic form for its integral over a period: 2pi/3.
    auto f = [](double x) { return 1/(5 - 4*cos(x)); };

    double Q = trapezoidal(f, (double) 0, two_pi<double>());

    ex_out << std::setprecision(std::numeric_limits<double>::digits10);
    ex_out << "The adaptive trapezoidal rule gives the integral of our function as " << Q << "\n";
    ex_out << "The exact result is                                                 " << two_pi<double>()*third<double>() << "\n";

    return 0;  // main's implicit return
}

TEST_CASE("example: trapezoidal_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(The adaptive trapezoidal rule gives the integral of our function as 2.0943951023932
The exact result is                                                 2.0943951023932
)ex"));
}
}  // namespace ex_trapezoidal_example
