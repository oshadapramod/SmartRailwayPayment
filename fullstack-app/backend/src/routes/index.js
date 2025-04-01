const express = require('express');
const router = express.Router();

// Define your routes here
router.get('/', (req, res) => {
    res.send('Welcome to the API');
});

// Export the function to set routes
module.exports = (app) => {
    app.use('/api', router);
};