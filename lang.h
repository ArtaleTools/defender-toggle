/* lang.h - UI strings. Every table lists: en, zh-Hant, ja, ko, es, fr, de, ru, pt.
 * Windows Security terms follow the wording Windows itself uses in each language. */
#define NLANG 9
enum { L_EN, L_ZH, L_JA, L_KO, L_ES, L_FR, L_DE, L_RU, L_PT };
static int g_lang;
#define TR(a) ((a)[g_lang])

/* Native names for the language menu, in enum order. */
static const wchar_t *const LANG_NAME[NLANG] = {
    L"English", L"繁體中文", L"日本語", L"한국어", L"Español",
    L"Français", L"Deutsch", L"Русский", L"Português" };

/* Han characters differ by region, so CJK languages each get their own font. */
static const wchar_t *const LANG_FACE[NLANG] = {
    L"Segoe UI", L"Microsoft JhengHei UI", L"Yu Gothic UI", L"Malgun Gothic", L"Segoe UI",
    L"Segoe UI", L"Segoe UI", L"Segoe UI", L"Segoe UI" };

/* Maps a Windows UI language to a table index. Chinese variants without their
 * own table use Traditional; anything unlisted falls back to English. */
static int lang_from_langid(LANGID id) {
    switch (PRIMARYLANGID(id)) {
    case LANG_CHINESE:    return L_ZH;
    case LANG_JAPANESE:   return L_JA;
    case LANG_KOREAN:     return L_KO;
    case LANG_SPANISH:    return L_ES;
    case LANG_FRENCH:     return L_FR;
    case LANG_GERMAN:     return L_DE;
    case LANG_RUSSIAN:    return L_RU;
    case LANG_PORTUGUESE: return L_PT;
    default:              return L_EN;
    }
}

static const wchar_t *const S_WINTITLE[NLANG] = {
    L"Defender Toggle", L"Defender 一鍵切換", L"Defender ワンクリック切替", L"Defender 원클릭 전환",
    L"Defender Toggle", L"Defender Toggle", L"Defender Toggle", L"Defender Toggle", L"Defender Toggle" };
static const wchar_t *const S_BTN_DISABLE[NLANG] = {
    L"Disable protection", L"關閉防護", L"保護をオフにする", L"보호 끄기",
    L"Desactivar protección", L"Désactiver la protection", L"Schutz ausschalten", L"Отключить защиту", L"Desativar proteção" };
static const wchar_t *const S_BTN_ENABLE[NLANG] = {
    L"Enable protection", L"開啟防護", L"保護をオンにする", L"보호 켜기",
    L"Activar protección", L"Activer la protection", L"Schutz einschalten", L"Включить защиту", L"Ativar proteção" };
static const wchar_t *const S_BTN_TAMPER_ON[NLANG] = {
    L"Disable Tamper Protection (open settings)",
    L"關閉竄改防護（開啟設定頁）",
    L"改ざん防止をオフにする（設定を開く）",
    L"변조 보호 끄기(설정 열기)",
    L"Desactivar protección contra alteraciones",
    L"Désactiver la protection anti-falsification",
    L"Manipulationsschutz ausschalten",
    L"Отключить защиту от подделки",
    L"Desativar proteção contra adulterações" };
static const wchar_t *const S_BTN_TAMPER_OFF[NLANG] = {
    L"Tamper Protection is off ✓",
    L"竄改防護已關閉 ✓",
    L"改ざん防止はオフです ✓",
    L"변조 보호 꺼짐 ✓",
    L"Protección contra alteraciones desactivada ✓",
    L"Protection anti-falsification désactivée ✓",
    L"Manipulationsschutz ist aus ✓",
    L"Защита от подделки отключена ✓",
    L"Proteção contra adulterações desativada ✓" };

static const wchar_t *const S_TAMPER_ALREADY_OFF[NLANG] = {
    L"Tamper Protection is already off.",
    L"竄改防護已經是關閉狀態。",
    L"改ざん防止はすでにオフです。",
    L"변조 보호가 이미 꺼져 있습니다.",
    L"La protección contra alteraciones ya está desactivada.",
    L"La protection contre les falsifications est déjà désactivée.",
    L"Der Manipulationsschutz ist bereits ausgeschaltet.",
    L"Защита от подделки уже отключена.",
    L"A proteção contra adulterações já está desativada." };
static const wchar_t *const S_TITLE_TAMPER[NLANG] = {
    L"Tamper Protection", L"竄改防護", L"改ざん防止", L"변조 보호",
    L"Protección contra alteraciones", L"Protection contre les falsifications",
    L"Manipulationsschutz", L"Защита от подделки", L"Proteção contra adulterações" };
static const wchar_t *const S_TITLE_DEFSWITCH[NLANG] = {
    L"Defender Toggle", L"Defender 切換", L"Defender 切替", L"Defender 전환",
    L"Defender Toggle", L"Defender Toggle", L"Defender Toggle", L"Defender Toggle", L"Defender Toggle" };
static const wchar_t *const S_TAMPER_OPENED[NLANG] = {
    L"Opened the \"Virus & threat protection settings\" page.\n\n"
    L"Please turn off \"Tamper Protection\",\n"
    L"then come back to this window and click the button again.\n\n"
    L"Note: Tamper Protection is locked by Microsoft; no program can turn it off "
    L"automatically. It can only be toggled manually on that page.",

    L"已開啟「病毒與威脅防護設定」頁。\n\n"
    L"請把「竄改保護 (Tamper Protection)」切為關閉，\n"
    L"再回到本視窗按「開啟/關閉防護」。\n\n"
    L"註：竄改防護是微軟鎖定的開關，任何程式都無法自動關閉，\n"
    L"只能在這個頁面手動撥動。",

    L"「ウイルスと脅威の防止の設定」ページを開きました。\n\n"
    L"「改ざん防止」をオフにしてから、\n"
    L"このウィンドウに戻ってもう一度ボタンを押してください。\n\n"
    L"注: 改ざん防止は Microsoft によりロックされており、プログラムからは"
    L"オフにできません。このページで手動で切り替える必要があります。",

    L"\"바이러스 및 위협 방지 설정\" 페이지를 열었습니다.\n\n"
    L"\"변조 보호\"를 끈 다음\n"
    L"이 창으로 돌아와 버튼을 다시 눌러 주세요.\n\n"
    L"참고: 변조 보호는 Microsoft가 잠근 설정이라 어떤 프로그램도 자동으로 끌 수 없으며, "
    L"이 페이지에서 직접 바꿔야 합니다.",

    L"Se abrió la página de configuración de \"Protección contra virus y amenazas\".\n\n"
    L"Desactive la \"Protección contra alteraciones\",\n"
    L"vuelva a esta ventana y pulse el botón otra vez.\n\n"
    L"Nota: Microsoft bloquea esta opción; ningún programa puede desactivarla "
    L"automáticamente. Solo se puede cambiar a mano en esa página.",

    L"La page des paramètres « Protection contre les virus et menaces » est ouverte.\n\n"
    L"Désactivez la « Protection contre les falsifications »,\n"
    L"puis revenez à cette fenêtre et cliquez à nouveau sur le bouton.\n\n"
    L"Remarque : Microsoft verrouille cette option ; aucun programme ne peut la désactiver "
    L"automatiquement. Elle se change uniquement à la main sur cette page.",

    L"Die Einstellungsseite „Viren- & Bedrohungsschutz“ wurde geöffnet.\n\n"
    L"Schalten Sie den „Manipulationsschutz“ aus,\n"
    L"kehren Sie dann zu diesem Fenster zurück und klicken Sie erneut auf die Schaltfläche.\n\n"
    L"Hinweis: Microsoft sperrt diese Option; kein Programm kann sie automatisch ausschalten. "
    L"Sie lässt sich nur auf dieser Seite von Hand ändern.",

    L"Открыта страница параметров «Защита от вирусов и угроз».\n\n"
    L"Отключите «Защиту от подделки»,\n"
    L"затем вернитесь в это окно и снова нажмите кнопку.\n\n"
    L"Примечание: Microsoft блокирует этот параметр, и ни одна программа не может отключить "
    L"его автоматически. Его можно изменить только вручную на этой странице.",

    L"A página de configurações de \"Proteção contra vírus e ameaças\" foi aberta.\n\n"
    L"Desative a \"Proteção contra adulterações\",\n"
    L"depois volte a esta janela e clique no botão novamente.\n\n"
    L"Observação: a Microsoft bloqueia esta opção; nenhum programa pode desativá-la "
    L"automaticamente. Ela só pode ser alterada manualmente nessa página." };

static const wchar_t *const S_BUILDING_TITLE[NLANG] = {
    L"Preparing boot image", L"正在準備開機映像", L"起動イメージを準備中", L"부팅 이미지 준비 중",
    L"Preparando imagen de arranque", L"Préparation de l'image de démarrage",
    L"Startabbild wird vorbereitet", L"Подготовка загрузочного образа", L"Preparando imagem de inicialização" };
static const wchar_t *const S_BUILDING_SUB[NLANG] = {
    L"First-time setup, takes a few minutes. Don't close this window.",
    L"第一次使用，約需幾分鐘，請不要關閉視窗",
    L"初回のみ数分かかります。ウィンドウを閉じないでください",
    L"처음 한 번만 몇 분 걸립니다. 창을 닫지 마세요",
    L"Solo la primera vez, unos minutos. No cierre esta ventana.",
    L"Première fois seulement, quelques minutes. Ne fermez pas la fenêtre.",
    L"Nur beim ersten Mal, einige Minuten. Fenster nicht schließen.",
    L"Только в первый раз, несколько минут. Не закрывайте окно.",
    L"Só na primeira vez, alguns minutos. Não feche esta janela." };
static const wchar_t *const S_PEND_DISABLE_TITLE[NLANG] = {
    L"Fully off after restart", L"重開後徹底關閉", L"再起動後に完全にオフ", L"다시 시작하면 완전히 꺼짐",
    L"Se desactivará al reiniciar", L"Désactivé au redémarrage",
    L"Nach Neustart ganz aus", L"Отключится после перезагрузки", L"Será desativado ao reiniciar" };
static const wchar_t *const S_PEND_RESTORE_TITLE[NLANG] = {
    L"Restored after restart", L"重開後恢復防護", L"再起動後に保護を復元", L"다시 시작하면 보호 복원",
    L"Se restaurará al reiniciar", L"Rétabli au redémarrage",
    L"Nach Neustart wieder aktiv", L"Включится после перезагрузки", L"Será restaurado ao reiniciar" };
static const wchar_t *const S_PEND_SUB[NLANG] = {
    L"Scheduled. It finishes automatically when you restart.",
    L"已排好，重開時自動完成，不用按任何東西",
    L"予約済みです。再起動時に自動で完了します",
    L"예약되었습니다. 다시 시작할 때 자동으로 완료됩니다",
    L"Programado. Se completará solo al reiniciar.",
    L"Planifié. Se termine automatiquement au redémarrage.",
    L"Geplant. Wird beim Neustart automatisch abgeschlossen.",
    L"Запланировано. Завершится само при перезагрузке.",
    L"Agendado. Será concluído sozinho ao reiniciar." };
static const wchar_t *const S_FULLOFF_TITLE[NLANG] = {
    L"Defender is fully off", L"Defender 已徹底關閉", L"Defender は完全にオフです", L"Defender 완전히 꺼짐",
    L"Defender desactivado", L"Defender désactivé", L"Defender ist ganz aus", L"Defender отключён", L"Defender desativado" };
static const wchar_t *const S_FULLOFF_SUB_PENDING[NLANG] = {
    L"Settings are ready; restart to actually stop it",
    L"設定已改好，重開才會真的停止",
    L"設定は完了しました。再起動すると停止します",
    L"설정을 마쳤습니다. 다시 시작하면 중지됩니다",
    L"Configuración lista; reinicie para detenerlo",
    L"Paramètres prêts ; redémarrez pour l'arrêter",
    L"Einstellungen fertig; zum Stoppen neu starten",
    L"Настройки готовы; перезагрузите, чтобы остановить",
    L"Configurações prontas; reinicie para interromper" };
static const wchar_t *const S_FULLOFF_SUB_DONE[NLANG] = {
    L"Real-time protection, services, tasks and tray icon are off",
    L"即時防護、服務、排程與小圖示皆已停用",
    L"リアルタイム保護・サービス・タスク・トレイアイコンは無効です",
    L"실시간 보호, 서비스, 작업, 트레이 아이콘이 꺼져 있습니다",
    L"Protección en tiempo real, servicios, tareas e icono: desactivados",
    L"Protection en temps réel, services, tâches et icône : désactivés",
    L"Echtzeitschutz, Dienste, Aufgaben und Tray-Symbol sind aus",
    L"Защита, службы, задачи и значок в трее отключены",
    L"Proteção em tempo real, serviços, tarefas e ícone: desativados" };
static const wchar_t *const S_RUNNING_TITLE[NLANG] = {
    L"Defender is running", L"Defender 運作中", L"Defender は動作中です", L"Defender 실행 중",
    L"Defender activo", L"Defender actif", L"Defender ist aktiv", L"Defender работает", L"Defender ativo" };
static const wchar_t *const S_RUNNING_SUB[NLANG] = {
    L"Real-time protection is normal", L"即時防護正常", L"リアルタイム保護は正常です", L"실시간 보호가 정상입니다",
    L"La protección en tiempo real funciona", L"La protection en temps réel fonctionne",
    L"Echtzeitschutz funktioniert", L"Защита в реальном времени работает", L"A proteção em tempo real está normal" };
static const wchar_t *const S_FULLON_SUB_PENDING[NLANG] = {
    L"Settings restored; restart to actually enable it",
    L"設定已還原，重開才會真的啟動",
    L"設定を元に戻しました。再起動すると有効になります",
    L"설정을 복원했습니다. 다시 시작하면 켜집니다",
    L"Configuración restaurada; reinicie para activarlo",
    L"Paramètres rétablis ; redémarrez pour l'activer",
    L"Einstellungen zurückgesetzt; zum Aktivieren neu starten",
    L"Настройки восстановлены; перезагрузите, чтобы включить",
    L"Configurações restauradas; reinicie para ativar" };
static const wchar_t *const S_PARTIAL_TITLE[NLANG] = {
    L"Defender is partly off", L"Defender 部分關閉", L"Defender は一部オフです", L"Defender 일부 꺼짐",
    L"Defender parcialmente activo", L"Defender partiellement actif",
    L"Defender ist teilweise aus", L"Defender отключён частично", L"Defender parcialmente ativo" };
static const wchar_t *const S_PARTIAL_SUB_RUNNING[NLANG] = {
    L"Real-time protection may be off, but services are still running",
    L"即時防護可能已關，但服務還在跑",
    L"リアルタイム保護はオフの可能性がありますが、サービスは動作中です",
    L"실시간 보호는 꺼졌을 수 있지만 서비스는 실행 중입니다",
    L"La protección puede estar desactivada, pero los servicios siguen activos",
    L"La protection est peut-être coupée, mais les services tournent encore",
    L"Echtzeitschutz evtl. aus, aber die Dienste laufen noch",
    L"Защита может быть отключена, но службы ещё работают",
    L"A proteção pode estar desativada, mas os serviços continuam ativos" };
static const wchar_t *const S_PARTIAL_SUB_OTHER[NLANG] = {
    L"Some service settings don't match. You can click the button again.",
    L"有服務設定不一致，可再按一次按鈕",
    L"一部のサービス設定が一致しません。もう一度ボタンを押せます",
    L"일부 서비스 설정이 맞지 않습니다. 버튼을 다시 누를 수 있습니다",
    L"Algunos servicios no coinciden. Puede pulsar el botón otra vez.",
    L"Certains services ne concordent pas. Vous pouvez recliquer sur le bouton.",
    L"Einige Dienste passen nicht. Sie können erneut klicken.",
    L"Настройки служб не совпадают. Можно нажать кнопку ещё раз.",
    L"Alguns serviços não coincidem. Você pode clicar novamente." };
static const wchar_t *const S_TAMPER_WARN[NLANG] = {
    L"⚠ Tamper Protection is on. Turn it off manually first.",
    L"⚠ 竄改防護開啟中，請先手動關閉再操作",
    L"⚠ 改ざん防止がオンです。先に手動でオフにしてください",
    L"⚠ 변조 보호가 켜져 있습니다. 먼저 직접 꺼 주세요",
    L"⚠ Protección contra alteraciones activa. Desactívela primero.",
    L"⚠ Protection anti-falsification active. Désactivez-la d'abord.",
    L"⚠ Manipulationsschutz ist an. Zuerst von Hand ausschalten.",
    L"⚠ Защита от подделки включена. Сначала отключите её.",
    L"⚠ Proteção contra adulterações ativa. Desative-a primeiro." };

/* Complete clauses, used as the first %s of the result messages below. */
static const wchar_t *const S_PHRASE_DISABLED[NLANG] = {
    L"Real-time protection is off",
    L"即時防護已關閉",
    L"リアルタイム保護をオフにしました",
    L"실시간 보호를 껐습니다",
    L"La protección en tiempo real está desactivada",
    L"La protection en temps réel est désactivée",
    L"Der Echtzeitschutz ist ausgeschaltet",
    L"Защита в реальном времени отключена",
    L"A proteção em tempo real está desativada" };
static const wchar_t *const S_PHRASE_RESTORED[NLANG] = {
    L"Real-time protection is restored",
    L"即時防護已恢復",
    L"リアルタイム保護を元に戻しました",
    L"실시간 보호를 복원했습니다",
    L"La protección en tiempo real se ha restaurado",
    L"La protection en temps réel est rétablie",
    L"Der Echtzeitschutz ist wiederhergestellt",
    L"Защита в реальном времени восстановлена",
    L"A proteção em tempo real foi restaurada" };
static const wchar_t *const S_FMT_SVC_NOT_SCHEDULED[NLANG] = {
    L"%s, but services were not scheduled: %s\n\nServices remain unchanged.",
    L"%s，但服務沒有排好：%s\n\n目前服務維持原狀。",
    L"%sが、サービスの変更を予約できませんでした: %s\n\nサービスは変更されていません。",
    L"%s. 하지만 서비스 변경을 예약하지 못했습니다: %s\n\n서비스는 변경되지 않았습니다.",
    L"%s, pero los servicios no se programaron: %s\n\nLos servicios no han cambiado.",
    L"%s, mais les services n'ont pas été planifiés : %s\n\nLes services restent inchangés.",
    L"%s, aber die Dienste wurden nicht eingeplant: %s\n\nDie Dienste bleiben unverändert.",
    L"%s, но изменение служб не запланировано: %s\n\nСлужбы остались без изменений.",
    L"%s, mas os serviços não foram agendados: %s\n\nOs serviços não foram alterados." };
static const wchar_t *const S_IMG_NOT_READY[NLANG] = {
    L"the boot image isn't ready yet.",
    L"開機映像還沒準備好。",
    L"起動イメージの準備ができていません。",
    L"부팅 이미지가 아직 준비되지 않았습니다.",
    L"la imagen de arranque aún no está lista.",
    L"l'image de démarrage n'est pas encore prête.",
    L"das Startabbild ist noch nicht bereit.",
    L"загрузочный образ ещё не готов.",
    L"a imagem de inicialização ainda não está pronta." };
static const wchar_t *const S_SCHED_FAILED[NLANG] = {
    L"failed to schedule for next boot.",
    L"排定下次開機失敗。",
    L"次回起動の予約に失敗しました。",
    L"다음 부팅 예약에 실패했습니다.",
    L"no se pudo programar para el próximo arranque.",
    L"échec de la planification au prochain démarrage.",
    L"die Planung für den nächsten Start ist fehlgeschlagen.",
    L"не удалось запланировать на следующую загрузку.",
    L"não foi possível agendar para a próxima inicialização." };
static const wchar_t *const S_FMT_DONE_NOREBOOT[NLANG] = {
    L"%s. No restart needed.",
    L"%s，不用重開。",
    L"%s。再起動は不要です。",
    L"%s. 다시 시작할 필요가 없습니다.",
    L"%s. No hace falta reiniciar.",
    L"%s. Aucun redémarrage nécessaire.",
    L"%s. Kein Neustart nötig.",
    L"%s. Перезагрузка не нужна.",
    L"%s. Não é preciso reiniciar." };
static const wchar_t *const S_FMT_REBOOT_ASK[NLANG] = {
    L"%s. %s\n\nRestart now? Otherwise it applies on your next restart.",
    L"%s。%s\n\n要現在重開嗎？不重開的話，下次重開才生效。",
    L"%s。%s\n\n今すぐ再起動しますか？しない場合は次回の再起動で反映されます。",
    L"%s. %s\n\n지금 다시 시작할까요? 그렇지 않으면 다음에 다시 시작할 때 적용됩니다.",
    L"%s. %s\n\n¿Reiniciar ahora? Si no, se aplicará en el próximo reinicio.",
    L"%s. %s\n\nRedémarrer maintenant ? Sinon, cela s'appliquera au prochain redémarrage.",
    L"%s. %s\n\nJetzt neu starten? Sonst wird es beim nächsten Neustart wirksam.",
    L"%s. %s\n\nПерезагрузить сейчас? Иначе изменения вступят в силу при следующей перезагрузке.",
    L"%s. %s\n\nReiniciar agora? Caso contrário, será aplicado na próxima reinicialização." };
static const wchar_t *const S_REST_ARMED[NLANG] = {
    L"The rest is scheduled.\nIt finishes automatically on restart (one extra restart), nothing else to do.",
    L"其餘設定已排好。\n重開時會自動完成（會多重開一次），不用按任何東西。",
    L"残りの設定は予約済みです。\n再起動時に自動で完了します（再起動が 1 回増えます）。操作は不要です。",
    L"나머지 설정은 예약되었습니다.\n다시 시작할 때 자동으로 완료되며(재시작 1회 추가), 따로 할 일은 없습니다.",
    L"El resto está programado.\nSe completará solo al reiniciar (un reinicio adicional); no hay que hacer nada más.",
    L"Le reste est planifié.\nCela se termine automatiquement au redémarrage (un redémarrage de plus), sans autre action.",
    L"Der Rest ist eingeplant.\nEr wird beim Neustart automatisch abgeschlossen (ein zusätzlicher Neustart), sonst ist nichts zu tun.",
    L"Остальное запланировано.\nЗавершится автоматически при перезагрузке (одна дополнительная перезагрузка), больше ничего делать не нужно.",
    L"O restante está agendado.\nSerá concluído sozinho ao reiniciar (uma reinicialização extra); não é preciso fazer mais nada." };
static const wchar_t *const S_REST_DONE[NLANG] = {
    L"The rest has also been changed; one restart will apply it.",
    L"其餘設定也已改好，重開一次就生效。",
    L"残りの設定も変更しました。1 回再起動すると反映されます。",
    L"나머지 설정도 변경했습니다. 한 번 다시 시작하면 적용됩니다.",
    L"El resto también se ha cambiado; se aplicará con un reinicio.",
    L"Le reste a aussi été modifié ; un redémarrage l'appliquera.",
    L"Der Rest wurde ebenfalls geändert; ein Neustart macht ihn wirksam.",
    L"Остальное тоже изменено; изменения вступят в силу после перезагрузки.",
    L"O restante também foi alterado; será aplicado com uma reinicialização." };
static const wchar_t *const S_FMT_WRITE_FAIL[NLANG] = {
    L"%d setting(s) failed to write (Tamper Protection may be on).\n\n"
    L"Go to Windows Security > Virus & threat protection > Manage settings, "
    L"turn off Tamper Protection, then try again.",

    L"有 %d 項設定寫入失敗（可能是竄改防護開啟中）。\n\n"
    L"請到 Windows 安全性 > 病毒與威脅防護 > 管理設定，"
    L"關閉竄改保護後再試。",

    L"%d 件の設定を書き込めませんでした（改ざん防止がオンの可能性があります）。\n\n"
    L"Windows セキュリティ > ウイルスと脅威の防止 > 設定の管理 で"
    L"改ざん防止をオフにしてから、もう一度お試しください。",

    L"설정 %d개를 쓰지 못했습니다(변조 보호가 켜져 있을 수 있습니다).\n\n"
    L"Windows 보안 > 바이러스 및 위협 방지 > 설정 관리에서 "
    L"변조 보호를 끈 다음 다시 시도해 주세요.",

    L"No se pudieron escribir %d ajustes (quizá la protección contra alteraciones esté activa).\n\n"
    L"Vaya a Seguridad de Windows > Protección contra virus y amenazas > Administrar la configuración, "
    L"desactive la protección contra alteraciones e inténtelo de nuevo.",

    L"Échec de l'écriture de %d paramètre(s) (la protection contre les falsifications est peut-être active).\n\n"
    L"Allez dans Sécurité Windows > Protection contre les virus et menaces > Gérer les paramètres, "
    L"désactivez la protection contre les falsifications, puis réessayez.",

    L"%d Einstellung(en) konnten nicht geschrieben werden (evtl. ist der Manipulationsschutz an).\n\n"
    L"Öffnen Sie Windows-Sicherheit > Viren- & Bedrohungsschutz > Einstellungen verwalten, "
    L"schalten Sie den Manipulationsschutz aus und versuchen Sie es erneut.",

    L"Не удалось записать параметры: %d (возможно, включена защита от подделки).\n\n"
    L"Откройте «Безопасность Windows» > «Защита от вирусов и угроз» > «Управление настройками», "
    L"отключите защиту от подделки и повторите попытку.",

    L"Falha ao gravar %d configuração(ões) (a proteção contra adulterações pode estar ativa).\n\n"
    L"Vá em Segurança do Windows > Proteção contra vírus e ameaças > Gerenciar configurações, "
    L"desative a proteção contra adulterações e tente novamente." };
static const wchar_t *const S_ASK_BUILD_IMAGE[NLANG] = {
    L"First-time setup: a boot image needs to be prepared first.\n"
    L"A copy of this PC's built-in recovery system will be made and configured.\n"
    L"This takes about 3-5 minutes and uses about 1 GB (%s).\n\n"
    L"Prepare it now?",

    L"第一次使用，要先準備重開時用的開機映像：\n"
    L"從這台電腦內建的修復系統複製一份並改好，\n"
    L"約需 3～5 分鐘，會占用約 1GB（%s）。\n\n"
    L"要現在準備嗎？",

    L"初回のみ、再起動時に使う起動イメージの準備が必要です。\n"
    L"この PC の回復環境をコピーして設定します。\n"
    L"約 3～5 分かかり、約 1 GB を使用します（%s）。\n\n"
    L"今すぐ準備しますか？",

    L"처음 한 번은 다시 시작할 때 쓸 부팅 이미지를 준비해야 합니다.\n"
    L"이 PC의 복구 시스템을 복사해 설정합니다.\n"
    L"약 3~5분 걸리고 약 1GB를 사용합니다(%s).\n\n"
    L"지금 준비할까요?",

    L"La primera vez hay que preparar una imagen de arranque.\n"
    L"Se copiará y configurará el sistema de recuperación de este PC.\n"
    L"Tarda unos 3-5 minutos y ocupa cerca de 1 GB (%s).\n\n"
    L"¿Prepararla ahora?",

    L"La première fois, une image de démarrage doit être préparée.\n"
    L"Le système de récupération de ce PC sera copié et configuré.\n"
    L"Cela prend environ 3 à 5 minutes et utilise environ 1 Go (%s).\n\n"
    L"La préparer maintenant ?",

    L"Beim ersten Mal muss ein Startabbild vorbereitet werden.\n"
    L"Das Wiederherstellungssystem dieses PCs wird kopiert und eingerichtet.\n"
    L"Das dauert etwa 3-5 Minuten und belegt etwa 1 GB (%s).\n\n"
    L"Jetzt vorbereiten?",

    L"В первый раз нужно подготовить загрузочный образ.\n"
    L"Среда восстановления этого ПК будет скопирована и настроена.\n"
    L"Это займёт 3-5 минут и около 1 ГБ (%s).\n\n"
    L"Подготовить сейчас?",

    L"Na primeira vez é preciso preparar uma imagem de inicialização.\n"
    L"O sistema de recuperação deste PC será copiado e configurado.\n"
    L"Leva cerca de 3 a 5 minutos e usa cerca de 1 GB (%s).\n\n"
    L"Preparar agora?" };
static const wchar_t *const S_BUILD_FAILED[NLANG] = {
    L"Failed to prepare the boot image. Services remain unchanged.\n\nDetails in %s\\build.log.",
    L"開機映像準備失敗，服務維持原狀。\n\n詳細原因在 %s\\build.log。",
    L"起動イメージの準備に失敗しました。サービスは変更されていません。\n\n詳細: %s\\build.log",
    L"부팅 이미지 준비에 실패했습니다. 서비스는 변경되지 않았습니다.\n\n자세한 내용: %s\\build.log",
    L"No se pudo preparar la imagen de arranque. Los servicios no han cambiado.\n\nDetalles en %s\\build.log.",
    L"Échec de la préparation de l'image de démarrage. Les services restent inchangés.\n\nDétails dans %s\\build.log.",
    L"Das Startabbild konnte nicht vorbereitet werden. Die Dienste bleiben unverändert.\n\nDetails in %s\\build.log.",
    L"Не удалось подготовить загрузочный образ. Службы остались без изменений.\n\nПодробности: %s\\build.log.",
    L"Falha ao preparar a imagem de inicialização. Os serviços não foram alterados.\n\nDetalhes em %s\\build.log." };
static const wchar_t *const S_BUILDING_CLOSE_WARN[NLANG] = {
    L"Preparing the boot image. Please don't close until it finishes.",
    L"正在準備開機映像，完成前請不要關閉。",
    L"起動イメージを準備中です。完了するまで閉じないでください。",
    L"부팅 이미지를 준비 중입니다. 끝날 때까지 닫지 마세요.",
    L"Preparando la imagen de arranque. No cierre hasta que termine.",
    L"Préparation de l'image de démarrage. Ne fermez pas avant la fin.",
    L"Das Startabbild wird vorbereitet. Bitte erst danach schließen.",
    L"Идёт подготовка загрузочного образа. Не закрывайте до завершения.",
    L"Preparando a imagem de inicialização. Não feche até terminar." };
