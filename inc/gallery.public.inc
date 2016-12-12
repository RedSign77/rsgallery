<?php

function rsg_pp_router($args) {
	drupal_add_js(drupal_get_path('module', 'rsgallery') . '/js/rsgallery.js');
	drupal_set_title('Galériák');
	$page = array();
	if (function_exists('rsg_pp_' . str_replace('-', '_', $args) . '_page')) {
		$page['module'] = call_user_func('rsg_pp_' . str_replace('-', '_', $args) . '_page');
	}
	else {
		drupal_set_title('Galéria: A keresett oldal nem található');
		drupal_set_message('A keresett oldal fejlesztés alatt lehet még vagy a link hibás.', 'warning');
		drupal_set_message('call: <strong>rsg_pp_' . str_replace('-', '_', $args) . '_page</strong>', 'info');
	}

	return $page;
}

function rsg_pp_all_page()
{
	drupal_add_css(drupal_get_path('module', 'rsgallery') . '/css/bootstrap.3.3.5.min.css', array('type' => 'file'));
	drupal_add_js('https://cdn.jsdelivr.net/bootstrap/3.3.5/js/bootstrap.min.js', array('type' => 'file'));
	$page = array();
	$galleries = db_select(RSG_TBL_GALLERY, 'galleries')
		->fields('galleries')
		->condition('status', 1)
		->execute();
	$page['gallery']['#markup'] = '<div class="gallery col-lg-12 col-md-12">';
	while ($row = $galleries->fetchAssoc()) {
		$image = file_load($row['image']);
		$page['gallery']['#markup'] .= '<a href="' . url(RSG_PUBLIC_ROOT . '/gallery/' . $row['id']) . '">';
		$page['gallery']['#markup'] .= '<div class="col-lg-4 col-md-4 col-sm-6 col-xs-12"><div class="thumbnail">';
		$page['gallery']['#markup'] .= '<img src="' . image_style_url('gallery_cover', $image->uri) . '" alt="' . $row['label'] . ' cover" class="img img-responsive img-thumbnail"/>';
		$page['gallery']['#markup'] .= '<div class="caption"><h4>' . $row['label'] . '</h4>';
		//$page['gallery']['#markup'] .= '<p class=""><span class="tag">'.implode('</span><span>', explode(', ', $row['tags'])).'</span></p>';
		$page['gallery']['#markup'] .= '</div></div></div></a>';
	}
	$page['gallery']['#markup'] .= '</div>';


	return $page;
}

function rsg_pp_gallery_page()
{
	drupal_add_css(drupal_get_path('module', 'rsgallery') . '/css/bootstrap.3.3.5.min.css', array('type' => 'file'));
	drupal_add_css(drupal_get_path('module', 'rsgallery') . '/css/ekko-lightbox.min.css', array('type' => 'file'));
	drupal_add_js('https://cdn.jsdelivr.net/bootstrap/3.3.5/js/bootstrap.min.js', array('type' => 'file'));
	drupal_add_js(drupal_get_path('module', 'rsgallery') . '/js/ekko-lightbox.min.js', array('type' => 'file'));
	$galleryId = (int)getVars(2);
	$page = array();
	$gallery = db_select(RSG_TBL_GALLERY, 'gallery')->fields('gallery')->condition('id', $galleryId)->execute()->fetchAssoc();
	drupal_set_title($gallery['label']);
	$breads = array(
		l('Címlap', '<front>'),
		l('Galériák', 'galleries/all'),
	);
	drupal_set_breadcrumb($breads);
	$query = db_select(RSG_TBL_PICTURE, 'picture')
		->fields('picture')
		->condition('gallery', $galleryId)
		->extend('PagerDefault')
		->limit(15);

	$result = $query->execute();
	if ($result->rowCount() > 0) {
		$page['pictures']['#markup'] = '<div class="gallery">';

		while ($row = $result->fetchAssoc()) {
			$image = file_load($row['fid']);
			$page['pictures']['#markup'] .= '<div class="col-lg-4 col-md-4 col-sm-6 col-xs-12">';
			$page['pictures']['#markup'] .= '<a href="' . file_create_url($image->uri) . '" data-toggle="lightbox" data-gallery="multiimages" data-title="" data-footer="&copy; Ringató mesevilág ' . date("Y", time()) . '">';
			$page['pictures']['#markup'] .= '<img src="' . image_style_url('gallery_picture', $image->uri) . '" class="img img-responsive img-thumbnail"/>';
			$page['pictures']['#markup'] .= '</a>';
			$page['pictures']['#markup'] .= '</div>';

		}
		$page['pictures']['#markup'] .= '</div>';
	}
	$page['pager']['#markup'] = theme('pager');
	$page['bottom']['#markup'] = '<div class="clearfix"></div>';
	return $page;
}