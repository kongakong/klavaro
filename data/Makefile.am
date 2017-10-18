# Process this file with automake to produce Makefile.in

SUBDIRS = icons

klavaro_data = \
	klavaro.glade \
	klavaro_icon.png \
	beep.png \
	fluid.png \
	go_www.png \
	key.png \
	other.png \
	progress.png \
	top10.png \
	hands_0.png \
	hands_1.png \
	hands_2.png \
	hands_3.png \
	hands_4.png \
	hands_5.png \
	hands_6.png \
	hands_7.png \
	hands_8.png \
	hands_9.png \
	basic_lessons.txt \
	fingers_position.txt \
	alphagrip5_us.kbd \
	azerty_be.kbd \
	azerty_fr.kbd \
	azerty_fr_ibook.kbd \
	colemak_us.kbd \
 	dubeolsik_kr.kbd \
	dvorak_bg.kbd \
	dvorak_br_nativo.kbd \
	dvorak_br_us.kbd \
	dvorak_ca_fr.kbd \
	dvorak_de_neo2.kbd \
	dvorak_eo_eurokeys.kbd \
	dvorak_eo_nativo.kbd \
	dvorak_eo_uk.kbd \
	dvorak_es_us.kbd \
	dvorak_es_nativo.kbd \
	dvorak_fr.kbd \
	dvorak_fr_bepo.kbd \
	dvorak_se_us.kbd \
	dvorak_tr_F.kbd \
	dvorak_uk.kbd \
	dvorak_us.kbd \
	dvorak_us_prog.kbd \
	gurmukhi_in_jhelum.kbd \
	gurmukhi_pk_jhelum.kbd \
	gurmukhi_pk_jhelumShift.kbd \
	jtsuken_kk.kbd \
	jtsuken_ru.kbd \
	jtsuken_ru_typewriter.kbd \
	jtsuken_ua.kbd \
	qwerty_ar.kbd \
	qwerty_bg.kbd \
	qwerty_bg_2.kbd \
	qwerty_br.kbd \
	qwerty_br_abnt2.kbd \
	qwerty_ca_multling.kbd \
	qwerty_ca_quebec.kbd \
	qwerty_cn_us.kbd \
	qwerty_cz.kbd \
	qwerty_dk.kbd \
	qwerty_gr.kbd \
	qwerty_gr_old.kbd \
	qwerty_es.kbd \
	qwerty_il.kbd \
	qwerty_il_old.kbd \
	qwerty_in.kbd \
	qwerty_it.kbd \
	qwerty_jp.kbd \
	qwerty_no.kbd \
	qwerty_pt.kbd \
	qwerty_se.kbd \
	qwerty_tr.kbd \
	qwerty_uk.kbd \
	qwerty_us.kbd \
	qwerty_us_2.kbd \
	qwerty_pk_crulp.kbd \
	qwerty_pk_nla.kbd \
	qwerty_pl_us.kbd \
	qwertz_ch_fr.kbd \
	qwertz_ch_de.kbd \
	qwertz_cz.kbd \
	qwertz_de.kbd \
	qwertz_hr.kbd \
	qwertz_hu.kbd \
	qwertz_pl.kbd \
	qwertz_rs.kbd \
	qwertz_sl.kbd \
	workman_us.kbd \
	C.words \
	C.paragraphs \
	ar.words \
	ar.paragraphs \
	bg.words \
	bg.paragraphs \
	bn.words \
	bn.paragraphs \
	ca.words \
	ca.paragraphs \
	cs.words \
	cs.paragraphs \
	da.words \
	da.paragraphs \
	de.words \
	de.paragraphs \
	el.words \
	el.paragraphs \
	fi.words \
	fi.paragraphs \
	en_GB.words \
	en_GB.paragraphs \
	eo.words \
	eo.paragraphs \
	es.words \
	es.paragraphs \
	eu.words \
	eu.paragraphs \
	fr.words \
	fr.paragraphs \
	gl.words \
	gl.paragraphs \
	hr.words \
	hr.paragraphs \
	hu.words \
	hu.paragraphs \
	it.words \
	it.paragraphs \
	kk.words \
	kk.paragraphs \
 	ko.words \
 	ko.paragraphs \
	nb.words \
	nb.paragraphs \
	nl.words \
	nl.paragraphs \
	pa.words \
	pa.paragraphs \
	pl.words \
	pl.paragraphs \
	pt_BR.words \
	pt_BR.paragraphs \
	ru.words \
	ru.paragraphs \
	sr.words \
	sr.paragraphs \
	sl.words \
	sl.paragraphs \
	sv.words \
	sv.paragraphs \
	uk.words \
	uk.paragraphs \
	ur.words \
	ur.paragraphs \
	vi.words \
	vi.paragraphs

pkgdata_DATA = $(klavaro_data)

if IS_WIN32
EXTRA_DIST = $(klavaro_data)
else
@INTLTOOL_DESKTOP_RULE@
desktopdir = $(prefix)/$(DATADIRNAME)/applications
desktop_in_file = klavaro.desktop.in
desktop_DATA = $(desktop_in_file:.desktop.in=.desktop)
@INTLTOOL_XML_RULE@
appdatadir = $(prefix)/$(DATADIRNAME)/appdata
appdata_in_files = klavaro.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
DISTCLEANFILES = $(desktop_DATA) \
                 $(appdata_DATA)
EXTRA_DIST = $(klavaro_data) \
	     $(desktop_in_file) \
	     $(appdata_in_files)

endif
