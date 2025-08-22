document.addEventListener('DOMContentLoaded', () => {
  const nodes = document.querySelectorAll('p, dd, div');
  nodes.forEach(el => {
    const t = (el.textContent || '').replace(/\s+/g,' ').trim();
    if (/^(Bases|Base):\s*object$/i.test(t)) el.style.display = 'none';
  });
});
