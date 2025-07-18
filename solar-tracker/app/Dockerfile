# Use official Nginx image as base
FROM nginx:alpine

# Install Python and curl for health check
RUN apk add --no-cache curl python3 py3-pip

# Create directory for our app
WORKDIR /app

# Copy webpage files
COPY index.html /usr/share/nginx/html/
COPY dish_position.txt /usr/share/nginx/html/

# Configure Nginx
RUN rm /etc/nginx/conf.d/default.conf
COPY nginx.conf /etc/nginx/conf.d/

# Copy Python script server
COPY spa /app/spa
RUN chmod +x /app/spa/script-server.py

# Expose ports
EXPOSE 1900 80

# Create a health check
HEALTHCHECK --interval=30s --timeout=3s \
  CMD curl -f http://localhost/ || exit 1

# Start script
COPY start_services.sh /app/
RUN chmod +x /app/start_services.sh

CMD ["/app/start_services.sh"]
