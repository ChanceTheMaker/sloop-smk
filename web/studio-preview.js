// SPDX-License-Identifier: GPL-3.0-only
// Theme-matched hover and keyboard previews for Studio links on the installer.
(() => {
  const I18N = window.SloopI18n;
  // Gallery cards enlarge in place; keep the floating preview for other Studio links.
  for (const caption of document.querySelectorAll('.screenshot-gallery figcaption')) {
    const description = document.createElement('span');
    description.className = 'gallery-description';
    description.append(...caption.childNodes);
    const action = document.createElement('span');
    action.className = 'gallery-action';
    action.dataset.i18n = 'ui.clickToPlayEdit';
    action.textContent = I18N.t('ui.clickToPlayEdit');
    caption.append(description, action);
  }
  const names = {stage:'Stage Red', matrix:'Matrix', dx:'Vintage DX7', modeld:'Model D Walnut',
    chocolate:'Chocolate Factory', vapor:'Vaporwave', midnight:'Midnight Studio', space:'Space Mission',
    bauhaus:'Bauhaus', ocean:'Ocean Lab', arcade:'Arcade ’84', hicon:'High Contrast'};
  const bubble = document.createElement('a');
  bubble.id = 'studio-preview';
  bubble.className = 'studio-preview';
  bubble.tabIndex = -1;
  bubble.hidden = true;
  const picture = document.createElement('img');
  picture.width = 720; picture.height = 550;
  const caption = document.createElement('div');
  caption.id = 'studio-preview-caption';
  const action = document.createElement('strong');
  action.className = 'studio-preview-action';
  bubble.append(picture, caption, action);
  document.body.append(bubble);
  let anchor = null, timer, previousDescription = null;
  function position() {
    if (!anchor) return;
    const rect = anchor.getBoundingClientRect();
    const hero = anchor.closest('.hero-actions');
    picture.style.maxHeight = '';
    let box = bubble.getBoundingClientRect();
    // Keep the hero preview outside the entire button row, including wrapped buttons.
    const bounds = hero ? hero.getBoundingClientRect() : rect;
    const spaceBelow = innerHeight - bounds.bottom - 18;
    const spaceAbove = bounds.top - 18;
    const below = hero
      ? spaceBelow >= box.height || (spaceAbove < box.height && spaceBelow >= spaceAbove)
      : rect.bottom + box.height + 12 <= innerHeight || rect.top < box.height + 12;
    if (hero) {
      const space = Math.max(0, below ? spaceBelow : spaceAbove);
      const chrome = box.height - picture.getBoundingClientRect().height;
      picture.style.maxHeight = Math.max(0, space - chrome) + 'px';
      box = bubble.getBoundingClientRect();
    }
    const left = Math.max(12, Math.min(rect.left + rect.width / 2 - box.width / 2, innerWidth - box.width - 12));
    const top = below ? bounds.bottom + 10 : bounds.top - box.height - 10;
    bubble.style.left = left + 'px';
    bubble.style.top = (hero ? top : Math.max(8, Math.min(top, innerHeight - box.height - 8))) + 'px';
    bubble.dataset.side = below ? 'below' : 'above';
    bubble.style.setProperty('--preview-arrow', Math.max(20, Math.min(rect.left + rect.width / 2 - left, box.width - 20)) + 'px');
  }
  function refresh() {
    if (!anchor) return;
    const root = document.documentElement;
    const params = new URL(anchor.href).searchParams;
    const requestedSkin = params.get('theme') || root.dataset.skin;
    const skin = Object.hasOwn(names, requestedSkin) ? requestedSkin : 'stage';
    const mode = (params.get('mode') || root.dataset.mode) === 'light' ? 'light' : 'dark';
    bubble.href = anchor.href;
    picture.src = document.body.dataset.studioPreviewPath
      ? document.body.dataset.studioPreviewPath.replace("{skin}", skin).replace("{mode}", mode)
      : `screenshots/previews/${skin}-${mode}.jpg`;
    picture.alt = I18N.t('ui.previewAlt',{theme:names[skin],mode:I18N.t('ui.'+mode)});
    caption.textContent = I18N.t('ui.previewAlt',{theme:names[skin],mode:I18N.t('ui.'+mode)});
    action.textContent = I18N.t('ui.clickToPlay');
    position();
  }
  function hide() {
    clearTimeout(timer);
    if (anchor) {
      if (previousDescription === null) anchor.removeAttribute('aria-describedby');
      else anchor.setAttribute('aria-describedby', previousDescription);
    }
    anchor = null; bubble.hidden = true;
  }
  function show(link) {
    clearTimeout(timer);
    if (anchor !== link) {
      hide(); anchor = link;
      previousDescription = link.getAttribute('aria-describedby');
      link.setAttribute('aria-describedby', [previousDescription, caption.id].filter(Boolean).join(' '));
    }
    bubble.classList.toggle('studio-preview-hero', !!link.closest('.hero-actions'));
    bubble.hidden = false; refresh();
  }
  const later = () => { clearTimeout(timer); timer = setTimeout(hide, 160); };
  for (const link of document.querySelectorAll('a[href="../editor/"], a[data-studio-preview]')) {
    if (link.closest('.site-nav')) continue;
    link.addEventListener('pointerenter', e => { if (e.pointerType !== 'touch') show(link); });
    link.addEventListener('pointerleave', later);
    link.addEventListener('focus', () => show(link));
    link.addEventListener('blur', later);
    link.addEventListener('click', hide);
  }
  bubble.addEventListener('pointerenter', () => clearTimeout(timer));
  bubble.addEventListener('pointerleave', later);
  picture.addEventListener('load', position);
  document.addEventListener('keydown', e => { if (e.key === 'Escape') hide(); });
  window.addEventListener('resize', position);
  window.addEventListener('scroll', () => {
    if (!anchor) return;
    const rect = anchor.getBoundingClientRect();
    if (rect.bottom < 0 || rect.top > innerHeight) hide();
    else position();
  }, {passive:true});
  I18N.onChange(refresh);
  new MutationObserver(refresh).observe(document.documentElement, {attributes:true, attributeFilter:['data-skin', 'data-mode']});
})();
