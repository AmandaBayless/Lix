/*
 * gameplay/win_game.cpp
 *
 */

#include <algorithm>
#include <sstream>

#include "win_game.h"

#include "../../api/manager.h"
#include "../../other/help.h"
#include "../../other/globals.h"
#include "../../other/user.h"
#include "../../level/level_me.h"
#include "../../level/level.h" // minimal sizes of level for window position

namespace Api {

const unsigned WindowGameplay::this_xl_network_results(300);
const unsigned WindowGameplay::this_yl_network_results(170);
const unsigned WindowGameplay::this_xl_single_results(300);
const unsigned WindowGameplay::this_yl_single_results(300);
const unsigned WindowGameplay::this_xl_no_results(200);
const unsigned WindowGameplay::this_yl_no_results(170);
const unsigned WindowGameplay::this_yl_net_no_results(140);
const unsigned WindowGameplay::top_item  (40);
const unsigned WindowGameplay::y_saved   (70);
const unsigned WindowGameplay::y_comment(100);
const unsigned WindowGameplay::y_button (200);

WindowGameplay::WindowGameplay(Replay* rep, const Level* const lev)
:
    Window(
     LEMSCR_X/2 - this_xl_no_results/2,
     (LEMSCR_Y - gloB->panel_gameplay_yl)/2
     - (lev ? this_yl_net_no_results : this_yl_no_results)/2,
     this_xl_no_results, lev ? this_yl_net_no_results : this_yl_no_results,
     Language::win_game_title),

    game_end(false),
    game_net(lev ? true : false),
    exit_with    (NOTHING),
    resume       (20, top_item     , this_xl_no_results-40),
    restart      (20, top_item + 30, this_xl_no_results-40),
    save_replay  (20, top_item + 60, this_xl_no_results-40),
    menu         (20, top_item + 90, this_xl_no_results-40),
    replay       (rep),
    level        (lev),
    browser_save (0)
{
    common_constructor();
    // Resume reagiert bedingt durch calc() auf jede Taste, die keinem
    // anderen Button im Fenster zugewiesen ist.

    // No hotkey during a network game, ESC should resume!
    if (game_net) {
        save_replay.set_y(top_item + 30);
        menu       .set_y(top_item + 60);
        resume .set_hotkey(useR->key_ga_exit);
        restart.set_hotkey();
        menu   .set_hotkey(useR->key_me_delete);

        on_key_me_exit = RESUME;
    }
}



WindowGameplay::WindowGameplay(
    Replay*            rep,
    const Tribe*       trlo,
    int                updates_used,
    int                _lix_required,
    int                _lix_at_start,
    const std::string& tit
) :
    Window(
     LEMSCR_X/2                             - this_xl_single_results/2,
     (LEMSCR_Y - gloB->panel_gameplay_yl)/2 - this_yl_single_results/2,
     this_xl_single_results, this_yl_single_results, tit),

    game_end(true),
    game_net(false),
    exit_with    (NOTHING),
    lix_saved    (trlo ? trlo->lix_saved : 0),
    lix_required (_lix_required),
    lix_at_start (_lix_at_start),
    resume       (20, y_button     , this_xl_single_results-40),
    restart      (20, y_button     , this_xl_single_results-40),
    save_replay  (20, y_button + 30, this_xl_single_results-40),
    menu         (20, y_button + 60, this_xl_single_results-40),
    replay       (rep),
    level        (0),
    browser_save (0)
{
    // This gets repeated in draw_self() for the big letters
    int col_b = color[COL_TEXT];
    if (lix_saved >= lix_required) col_b = color[COL_TEXT_ON];

    std::string c                         = Language::win_game_comment_none;
    if      (lix_saved == lix_at_start) c = Language::win_game_comment_perfect;
    else if (lix_saved >  lix_required) c = Language::win_game_comment_more;
    else if (lix_saved == lix_required) c = Language::win_game_comment_exactly;
    else if (lix_saved >  0)            c = Language::win_game_comment_less;

    labels.push_back(Label(get_xl()/2, top_item,
     trlo && trlo->lix_saved_late != 0 ? Language::win_game_lix_saved_in_time
                                       : Language::win_game_lix_saved,
                                         Label::CENTERED));
    labels.push_back(Label(get_xl()/2, y_comment, c,
     Label::CENTERED));
    labels.back().set_color(col_b);

    labels.push_back(Label(20, y_comment + 40,
     Language::win_game_result_skills));
    labels.push_back(Label(20, y_comment + 60,
     Language::win_game_result_time));

    labels.push_back(Label(get_xl() - 20, y_comment + 40,
                           trlo ? trlo->skills_used : 0));
    const int secs = updates_used / gloB->updates_per_second;

    std::ostringstream timestr;
    timestr    << secs / 60
        << ':' << (secs % 60 < 10 ? "0" : "") << secs % 60;
    labels.push_back(Label(get_xl() - 20, y_comment + 60,
                     timestr.str(), Label::BLOCKY));

    common_constructor();

    if (lix_saved >= lix_required)
         on_key_me_okay = MENU;
    else on_key_me_okay = RESTART;
}



WindowGameplay::WindowGameplay(
    Replay*                    rep,
    const std::vector <Tribe>& pv,
    const Tribe*               pl,
    const bool                 spec,
    const Level* const         lev)
:
    Window(LEMSCR_X/2 - this_xl_network_results/2,
           (LEMSCR_Y - gloB->panel_gameplay_yl)/2
                     - this_yl_network_results/2 - pv.size() * 10,
            this_xl_network_results,
            this_yl_network_results + pv.size() * 20, lev->get_name()),
    game_end(true),
    game_net(true),
    exit_with    (NOTHING),
    resume       (),
    restart      (),
    save_replay  (20, get_yl() - 70, get_xl() - 40),
    menu         (20, get_yl() - 40, get_xl() - 40),
    replay       (rep),
    level        (lev),
    browser_save (0)
{
    // Ergebnisse auswerten, sortieren und Listenerstellung vorbereiten
    std::vector <SortableTribe> sortvec;
    for (Tribe::CIt i = pv.begin(); i != pv.end(); ++i)
     sortvec.push_back(SortableTribe(&*i == pl && ! spec, &*i, i->lix_saved));
    std::sort(sortvec.begin(), sortvec.end());
    // Komische Abkuerzungen: o = own, s = score, p = place
    // These will be used to select the award sound later, but only
    // if spec is not set.
    unsigned op = 0;
    unsigned os = 0;

    // Liste schreiben
    for (unsigned place = 0; place < sortvec.size(); ++place) {
        std::ostringstream str;
        for (unsigned shown = 0; shown <= place; ++shown)
         if (sortvec[place].score == sortvec[shown].score) {
            str << shown + 1;
            break;
        }
        str << ". " << sortvec[place].tr->get_name();
        const int col = sortvec[place].pl ? color[COL_WHITE] : color[COL_TEXT];
        labels.push_back(Label(60, 40 + place*20, str.str()));
        labels.back().set_color(col);
        labels.push_back(Label(get_xl() - 60, 40 + place*20,
         sortvec[place].score));
        labels.back().set_color(col);
        // Dies nur fuer den Kommentar unten.
        if (sortvec[place].pl) {
            op = place;
            os = sortvec[place].score;
        }
    }

    // Kommentar schreiben
    std::string    c;
    const unsigned fs =    sortvec. begin() ->score;
    const unsigned ss = (++sortvec. begin())->score;
    const unsigned ls =    sortvec.rbegin() ->score;
    const unsigned lp =    sortvec.size()   - 1;

    if (! spec) {
        if      (fs == 0)             c = Language::win_game_net_all_zero;
        else if (os == 0)             c = Language::win_game_net_zero;
        else if (fs == ls)            c = Language::win_game_net_all_tie;
        else if (op == 0 && os == ss) c = Language::win_game_net_first_tie;
        else if (op < lp && os == ls) c = Language::win_game_net_last_tie;
        else if (op == 0)             c = Language::win_game_net_first;
        else if (op == lp)            c = Language::win_game_net_last;
        else if (op == 1)             c = Language::win_game_net_second;
        else                          c = Language::win_game_net_middle;
    }
    else {
        const bool team_win     = (sortvec.begin()->tr->masters.size() != 1);
        const std::string names =  sortvec.begin()->tr->get_name() + " ";

        if      (fs == ss) c =         Language::win_game_replay_tie;
        else if (team_win) c = names + Language::win_game_replay_win_team;
        else               c = names + Language::win_game_replay_win_one;
    }

    labels.push_back(Label(get_xl()/2, get_yl() - 110, c, Label::CENTERED));
    labels.back().set_color(color[COL_WHITE]);

    if (c == Language::win_game_net_first) {
        Sound::play_loud(Sound::AWARD_1);
    }
    else if (c == Language::win_game_net_second
     ||      c == Language::win_game_net_first_tie) {
        Sound::play_loud(Sound::AWARD_2);
    }
    else if (c == Language::win_game_net_middle
     ||      c == Language::win_game_net_all_tie) {
        Sound::play_loud(Sound::AWARD_3);
    }
    else if (! spec) Sound::play_loud(Sound::AWARD_4);

    common_constructor();

    menu.set_text  (Language::ok);
    on_key_me_okay = MENU;
}



WindowGameplay::~WindowGameplay()
{
    if (browser_save) delete browser_save;
}



void WindowGameplay::common_constructor()
{
    if (!game_end) add_child(resume);
    if (!game_net) add_child(restart);
    add_child(save_replay);
    add_child(menu);
    for (std::vector <Label> ::iterator itr = labels.begin();
     itr != labels.end(); ++itr) add_child(*itr);

    resume     .set_text(Language::win_game_resume);
    restart    .set_text(Language::win_game_restart);
    save_replay.set_text(Language::win_game_save_replay);
    menu       .set_text(Language::win_game_menu);
    restart    .set_hotkey(useR->key_restart);
    save_replay.set_hotkey(useR->key_me_export);
    menu       .set_hotkey(useR->key_ga_exit);

    on_key_me_okay = RESUME;
    on_key_me_exit = MENU;
}



void WindowGameplay::calc_self()
{
    if (browser_save) {
        if (browser_save->get_exit()) {
            if (browser_save->get_exit_with()) {
                replay->save_to_file(browser_save->get_current_file(), level);
                useR->replay_last_level = browser_save->get_current_file();
                Sound::play_loud(Sound::DISKSAVE);

                if (game_end) exit_with = MENU;
                else          exit_with = RESUME;
                Manager::remove_focus(this);
            }
            delete browser_save;
            browser_save = 0;
            set_draw_required();
        }
    }
    else {
        if (!game_net && restart.get_clicked()) {
            exit_with = RESTART;
            Manager::remove_focus(this);
        }
        else if (menu.get_clicked()) {
            exit_with = MENU;
            Manager::remove_focus(this);
        }
        else if (save_replay.get_clicked()) {
            browser_save = new SaveBrowser(
             gloB->dir_replay,
             gloB->ext_replay,
             useR->replay_last_level,
             search_criterion_save_replay,
             new_box_overwrite, true); // true = benutze Replay-Style
            browser_save->set_info_filename(replay->get_level_filename());
            browser_save->set_info_level_name(
             LevelMetaData(replay->get_level_filename()).get_name());
            browser_save->set_texttype(replay->get_canonical_save_filename());
            Manager::add_focus(browser_save);
        }
        else if (hardware.key_once(useR->key_me_exit)) {
            exit_with = on_key_me_exit;
        }
        else if (hardware.key_once(useR->key_me_okay)) {
            exit_with = on_key_me_okay;
        }
        else if (!game_end) {
            const int k = hardware.get_key();
            if (resume.get_clicked() || hardware.get_mr()
             || (k != -1 && k != menu       .get_hotkey()
                         && k != restart    .get_hotkey()
                         && k != save_replay.get_hotkey())) {
                exit_with = RESUME;
                Manager::remove_focus(this);
            }
        }
    }
}



bool WindowGameplay::search_criterion_save_replay(const Filename& fn)
{
    return fn.get_extension() == gloB->ext_replay;
}



Api::BoxMessage* WindowGameplay::new_box_overwrite(const Filename& file)
{
    Replay r;
    r.load_from_file(file);
    std::string s1 = Language::editor_file_name + ' ' + file.get_rootless();
    std::string s2 = Language::editor_level_name
                   + ' ' + LevelMetaData(r.get_level_filename()).get_name();
    Api::BoxMessage* box_overwrite = new Api::BoxMessage(500, 3,
                                     Language::win_game_overwrite_title);
    box_overwrite->add_text(Language::win_game_overwrite_question);
    box_overwrite->add_text(s1);
    box_overwrite->add_text(s2);
    box_overwrite->add_button(Language::yes, useR->key_me_okay);
    box_overwrite->add_button(Language::no,  useR->key_me_exit);
    return box_overwrite;
}



void WindowGameplay::draw_self()
{
    // This is a bit kludgy. Delete the remainders of the browser.
    rectfill(get_ground().get_al_bitmap(), 0, 0, LEMSCR_X-1, LEMSCR_Y
     - gloB->panel_gameplay_yl - 1,
     color[COL_PINK]);

    Window::draw_self();

    if (game_end && !game_net) {
        std::ostringstream s1; s1 << lix_saved;
        std::string        s2      = "/";
        std::ostringstream s3; s3 << lix_required;

        // This gets repeated in the corresponding constructor for this stuff
        int col_a = color[COL_TEXT];
        int col_b = color[COL_TEXT];
        if (lix_saved > 0)             col_a = color[COL_TEXT_ON];
        if (lix_saved >= lix_required) col_b = color[COL_TEXT_ON];

        Help::draw_shadow_centered_text(get_ground(), font_big,
         s1.str().c_str(), get_x() + get_xl()/2 - 40, get_y() + y_saved, col_a,
         color[COL_API_SHADOW]);
        Help::draw_shadow_centered_text(get_ground(), font_big,
         s2.c_str(),       get_x() + get_xl()/2,      get_y() + y_saved, col_b,
         color[COL_API_SHADOW]);
        Help::draw_shadow_centered_text(get_ground(), font_big,
         s3.str().c_str(), get_x() + get_xl()/2 + 40, get_y() + y_saved, col_b,
         color[COL_API_SHADOW]);
    }
}

} // Ende Namensraum Api
