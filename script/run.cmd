cp db_app.service /etc/systemd/system/
cp db_app.timer /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable db_app.timer
sudo systemctl start db_app.timer
sudo systemctl list-timers  # Check if it's active