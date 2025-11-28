// MADOLA Web App - Main Application Entry Point
// This file coordinates the loading sequence for the split application

// First, load the core application (constructor and main initialization)
console.log('Loading MADOLA core application...');

// Second, load the features extension after core is loaded
console.log('Loading MADOLA features extension...');
// Features will be loaded automatically via script tag order

// Main application will be initialized by app-ui.js DOMContentLoaded listener