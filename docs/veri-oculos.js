const topbar = document.querySelector('.topbar');
const menuToggle = document.querySelector('.menu-toggle');
const navLinks = document.querySelector('.nav-links');

menuToggle.addEventListener('click', () => {
  topbar.classList.toggle('open');
});

navLinks.querySelectorAll('a').forEach((link) => {
  link.addEventListener('click', () => {
    topbar.classList.remove('open');
  });
});

const faqItems = document.querySelectorAll('.faq-item');
faqItems.forEach((item) => {
  const button = item.querySelector('.faq-question');
  button.addEventListener('click', () => {
    const isOpen = item.classList.contains('open');
    faqItems.forEach((i) => i.classList.remove('open'));
    if (!isOpen) {
      item.classList.add('open');
    }
  });
});
