<?php

function rsg_ap_router($args) {
    drupal_add_library('system', 'ui.datepicker');
    drupal_add_js(drupal_get_path('module', 'rsgallery') . '/js/rsgallery.js');
    drupal_set_title('Gallery administration');
    $page = array();
    if (function_exists('rsg_' . str_replace('-', '_', $args) . '_page')) {
        $page['module'] = call_user_func('rsg_' . str_replace('-', '_', $args) . '_page');
    }
    else {
        drupal_set_title('Gallery: The requested page not found.');
        drupal_set_message('The requested page not found or it is under development.', 'warning');
        drupal_set_message('call: <strong>rsg_' . str_replace('-', '_', $args) . '_page</strong>', 'info');
    }

    return $page;
}

function rsg_galleries_page()
{
    drupal_set_title(t('Galleries'));
    $page = array();
    $header = array(
        array(
            'data'  => 'ID',
            'field' => 'id'
        ),
        array(
            'data'  => t('Created'),
            'field' => 'created_at'
        ),
        array(
            'data'  => t('Updated'),
            'field' => 'updated_at'
        ),
        array(
            'data'  => t('Name'),
            'field' => 'label'
        ),
        array(
            'data'  => t('Status'),
            'field' => 'status'
        ),
        array('data' => t('Options')),
    );
    $query = db_select(RSG_TBL_GALLERY, 'galleries')->fields('galleries');
    $query->extend('TableSort')
        ->orderByHeader($header);
    $query->extend('PagerDefault')
        ->limit(20);
    $result = $query->execute();
    $rows = array();
    while ($row = $result->fetchAssoc()) {
        $rows[] = array(
            $row['id'],
            $row['created_at'],
            $row['updated_at'],
            '<a href="' . url(RSG_ADMIN_ROOT . '/gallery/' . $row['id']) . '">' . $row['label'] . '</a>',
            $row['status'] > 0 ? t('Active') : t('Inactive'),
            '<a href="' . url(RSG_ADMIN_ROOT . '/uploader/' . $row['id']) . '">' . t('Manage images') . '</a>',
        );
    }
    $buttons = generate_bootstrap_buttons(array(
        RSG_ADMIN_ROOT . '/gallery/add' => array(
            'title' => t('Add new Gallery'),
            'style' => 'btn-primary btn-danger'
        )
    ));
    $page['buttons']['#markup'] = $buttons;
    $page['table']['#markup'] = theme('table', array(
            'caption' => t('Galleries'),
            'empty'   => t('Sorry, no gallery found in database'),
            'header'  => $header,
            'rows'    => $rows
        )) . theme('pager');


    return $page;
}

function rsg_gallery_page()
{
    $galleryId = (int) getVars(3);
    $form = drupal_get_form('am_gallery_form', array('id' => $galleryId));
    $page['#markup'] = render($form);

    return $page;
}

function am_gallery_form($form, &$form_state, $args) {
    $data = null;
    $galleryId = $args['id'];
    if ($galleryId > 0) {
        $data = db_select(RSG_TBL_GALLERY, 'gallery')
            ->fields('gallery')
            ->condition('id', $galleryId)
            ->execute()
            ->fetchAssoc();
    }
    $form = array();
    $form['id'] = array(
        '#type'          => 'hidden',
        '#default_value' => $galleryId,
    );
    $form['label'] = array(
        '#type'          => 'textfield',
        '#default_value' => $data === null ? '' : $data['label'],
        '#title'         => t('Label'),
        '#required'      => true,
    );
    $form['image'] = array(
        '#type'            => 'managed_file',
        '#upload_location' => 'public://rsgallery/gallery/',
        '#title'           => t('Gallery Cover Image'),
        '#required'        => false,
        '#default_value'   => $data === null ? null : (int) $data['image'],
        '#description'     => t('Please <strong>upload</strong> the picture before save the service!'),
    );
    $form['status'] = array(
        '#type'          => 'checkbox',
        '#title'         => t('Opened gallery'),
        '#default_value' => $data === null ? '' : $data['status'],
    );
    $form['tags'] = array(
        '#type'          => 'textfield',
        '#default_value' => $data === null ? '' : $data['tags'],
        '#title'         => t('Tags'),
        '#required'      => false,
    );
    $form['submit'] = array(
        '#type'          => 'submit',
        '#default_value' => $data === null ? 'Hozzáadás' : 'Mentés',
    );
    $form['cancel'] = array(
        '#type'          => 'submit',
        '#default_value' => t('Cancel'),
    );
    if ($data !== null) {
        $form['delete'] = array(
            '#type'          => 'submit',
            '#default_value' => t('Delete'),
        );
    }

    return $form;
}

function am_gallery_form_validate($form, &$form_state) {
    if ($form_state['values']['op'] === t('Cancel')) {
        drupal_set_message(t('The changes of gallery not saved.'));
        drupal_goto(RSG_ADMIN_ROOT . '/galleries');
    }

}

function am_gallery_form_submit($form, &$form_state) {
    $val = $form_state['values'];
    if ($val['op'] === t('Delete')) {
        db_delete(RSG_TBL_GALLERY)
            ->condition('id', $val['id'])
            ->execute();
        drupal_set_message(t($val['name'] . ' gallery deleted successfully.'));
    }
    else {
        _update_insert_gallery($val);
    }
    drupal_goto(RSG_ADMIN_ROOT . '/galleries');
}

function _update_insert_gallery($data) {
    global $user;
    $data['image'] = (int) $data['image'];
    $image = file_load($data['image']);
    $image->status = FILE_STATUS_PERMANENT;
    file_usage_add($image, 'rsgallery', 'gallery', $data['id']);
    file_save($image);
    if ((int) $data['id'] === 0) {
        // insert data
        $query = db_insert(RSG_TBL_GALLERY)->fields(array(
            'created_at',
            'updated_at',
            'label',
            'image',
            'status',
            'tags',
            'ip',
            'user_id'
        ));
        $query->values(array(
            'created_at' => date('Y-m-d H:i:s', time()),
            'updated_at' => date('Y-m-d H:i:s', time()),
            'label'      => $data['label'],
            'ip'         => ip_address(),
            'user_id'    => $user->uid,
            'tags'       => $data['tags'],
            'image'      => $data['image'],
            'status'     => $data['status'],
        ));
        $id = $query->execute();
        drupal_set_message('Gallery added successfully with id: ' . $id);
    }
    else {
        db_update(RSG_TBL_GALLERY)
            ->fields(array(
                'updated_at' => date('Y-m-d H:i:s', time()),
                'label'      => $data['label'],
                'tags'       => $data['tags'],
                'image'      => $data['image'],
                'status'     => $data['status'],
                'ip'         => ip_address(),
                'user_id'    => $user->uid,
            ))
            ->condition('id', $data['id'])
            ->execute();
        drupal_set_message('Gallery "' . $data['label'] . '" updated successfully');
    }

}

function rsg_uploader_page() {
    drupal_add_css(drupal_get_path('module', 'rsgallery') . '/css/ekko-lightbox.min.css', array('type' => 'file'));
    drupal_add_js(drupal_get_path('module', 'rsgallery') . '/js/ekko-lightbox.min.js', array('type' => 'file'));
    $galleryId = (int) getVars(3);
    drupal_set_title('Képek feltöltése');
    $page = array();
    $uploadForm = drupal_get_form('am_gallery_multiuploader_form', array('id' => $galleryId));
    $page['upload']['#markup'] = render($uploadForm);
    $page['gallery']['#markup'] = '<div class="gallery col-lg-12 col-md-3"><h2>' . t('Pictures of gallery') . '</h2>';
    $query = db_select(RSG_TBL_PICTURE, 'picture')
        ->fields('picture')
        ->condition('gallery', $galleryId)
        ->execute();
    if ($query->rowCount() > 0) {
        while ($row = $query->fetchAssoc()) {
            $_deleteForm = drupal_get_form('am_delete_picture_form', array('id' => $row['id'], 'fid' => $row['fid']));
            $pictureUrl = file_create_url($row['image']);
            $page['gallery']['#markup'] .= '<div class="picture col-lg-3 col-md-3">';
            $page['gallery']['#markup'] .= '<a href="'.$pictureUrl.'" data-toggle="lightbox" data-gallery="multiimages" data-label="">';
            $page['gallery']['#markup'] .= '<img class="img img-responsive img-thumbnail" src="' . image_style_url('gallery_picture', $row['image']) . '">';
            $page['gallery']['#markup'] .= '</a>';
            $page['gallery']['#markup'] .= render($_deleteForm);
            $page['gallery']['#markup'] .= '</div>';

        }
    }
    else {
        drupal_set_message('No uploaded images found in the gallery.', 'info');
    }
    $page['gallery']['#markup'] .= '</div>';

    return $page;
}

function am_gallery_multiuploader_form($form, $form_state, $args) {
    $form = array();
    $form['gallery'] = array(
        '#type'          => 'hidden',
        '#default_value' => $args['id']
    );
    $form['images'] = array(
        '#type'        => 'file',
        '#name'        => 'files[]',
        '#title'       => t('Select images for upload'),
        '#description' => t('Only JPG and PNG images with a maximum 2MB size.'),
        '#attributes'  => array('multiple' => 'multiple'),
    );
    $form['submit'] = array(
        '#type'          => 'submit',
        '#default_value' => t('Upload images'),
    );

    return $form;
}

function am_gallery_multiuploader_form_validate($form, &$form_state) {
    //Save multiple files
    $num_files = count($_FILES['files']['name']);
    for ($i = 0; $i < $num_files; $i++) {
        $file = file_save_upload($i, array(
            'file_validate_is_image'   => array(),
            'file_validate_extensions' => array('png jpg jpeg'),
        ));
        if ($file) {
            if ($file = file_move($file, 'public://rsgallery/images/')) {
                $form_state['values']['file'][$i] = $file;
            }
            else {
                form_set_error('file', t('Failed to write the uploaded file the site\'s/rsgallery/images/ folder.'));
            }
        }
        else {
            form_set_error('file', t('No file was uploaded.'));
        }
    }
}

function am_gallery_multiuploader_form_submit($form, &$form_state) {
    global $user;
    $galleryId = $form_state['values']['gallery'];
    if (is_array($form_state['values']['file'])) {
        //Make sure we keep our previously uploaded images
        foreach ($form_state['values']['file'] as $file) {
            $newPicture = file_load($file->fid);
            $newPicture->status = FILE_STATUS_PERMANENT;
            file_save($newPicture);
            $query = db_insert(RSG_TBL_PICTURE)->fields(array(
                'created_at',
                'gallery',
                'user_id',
                'ip',
                'image',
                'fid'
            ));
            $query->values(array(
                'created_at' => date("Y-m-d H:i:s", time()),
                'gallery'    => $galleryId,
                'user_id'    => $user->uid,
                'ip'         => ip_address(),
                'image'      => $newPicture->uri,
                'fid'		 => $newPicture->fid,
            ));
            $pictureId = $query->execute();
            watchdog('rsgallery', 'Image uploaded: ' . $pictureId);
        }
    }
}

function am_delete_picture_form($form, &$form_state, $args) {
    $form = array();
    $form['id'] = array(
        '#type'          => 'hidden',
        '#default_value' => $args['id']
    );
    $form['fid'] = array(
        '#type'          => 'hidden',
        '#default_value' => $args['fid']
    );
    $form['submit'] = array(
        '#type'          => 'submit',
        '#default_value' => t('Delete picture'),
    );

    return $form;
}

function am_delete_picture_form_submit($form, &$form_state)
{
    $image = file_load($form_state['values']['fid']);
    if (db_delete(RSG_TBL_PICTURE)->condition('id', $form_state['values']['id'])->execute()) {
        if ($image) {
            file_delete($image, true);
        }
        drupal_set_message(t('Picture deleted.'), 'info');
    } else {
        drupal_set_message(t('Sorry, the picture not deleted.'), 'error');
    }
}